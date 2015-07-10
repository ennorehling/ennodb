PREFIX = /opt
CFLAGS = -g -Wall -Werror -Wextra -Iiniparser -Icritbit
PROGRAMS = ennodb
TESTS = tests
WEBSITE = $(PREFIX)/share/ennodb/www/

ifeq "$(CC)" "clang"
CFLAGS += -Wno-padded -Wno-sign-conversion
endif

# http://www.thinkplexx.com/learn/howto/build-chain/make-based/prevent-gnu-make-from-always-removing-files-it-says-things-like-rm-or-removing-intermediate-files
.SECONDARY: prefix.o

all: $(PROGRAMS) $(TESTS)

test: $(TESTS)
	@./tests

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

%.test.o: %.test.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES) -DMOCKFCGI 

CuTest.o: critbit/CuTest.c
	$(CC) $(CFLAGS) -Wno-format-nonliteral -o $@ -c $< $(INCLUDES)

critbit.o: critbit/critbit.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

iniparser.o: iniparser/iniparser.c
	$(CC) $(CFLAGS) -Wno-unused-macros -o $@ -c $< $(INCLUDES)

cgiapp.a: cgiapp.o critbit.o iniparser.o
	$(AR) -q $@ $^

ennodb: ennodb.o nosql.o cgiapp.a
	$(CC) $(CFLAGS) -o $@ $^ -lfcgi $(LDFLAGS)

tests: ennodb-test.o mockfcgi.o tests.o nosql.o critbit/test_critbit.o CuTest.o critbit.o iniparser.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *~ *.a *.o */*.o $(PROGRAMS) $(TESTS)

install: $(PROGRAMS)
	sudo mkdir -p $(PREFIX)/bin
	sudo mkdir -p $(PREFIX)/share/doc/ennodb/examples
	sudo mkdir -p /var/lib/ennodb /etc/ennodb $(WEBSITE)
	sudo chown www-data.www-data /var/lib/ennodb
	sudo install html/*.* $(WEBSITE)
	sudo install $(PROGRAMS) $(PREFIX)/bin
	sudo install etc/init.d/* /etc/init.d/
	sudo install etc/ennodb/* /etc/ennodb/
	sudo install doc/examples/* $(PREFIX)/share/doc/ennodb/examples/

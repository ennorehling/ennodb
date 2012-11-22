CFLAGS = -Wall
BINS = build/bin/ennodb-fcgi

all: $(BINS)

build:
	mkdir -p build

build/Makefile: | build
	cd build ; cmake ..

build/bin/ennodb-fcgi: build/Makefile
	cd build ; make

install: $(BINS)
	sudo install $(BINS) /var/www/cgi-bin

CFLAGS = -Wall
BINS = build/bin/tkv-fcgi

all: $(BINS)

build:
	mkdir -p build

build/Makefile: | build
	cd build ; cmake ..

build/bin/tkv-fcgi: build/Makefile
	cd build ; make

install: $(BINS)
	sudo install $(BINS) /var/www/cgi-bin

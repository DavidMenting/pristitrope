INSTALL = /usr/bin/install
BINDIR = /usr/local/bin

all:
	gcc -Wall -o build/pristitrope pristitrope.c -lwiringPi -std=c99

install:	all
	$(INSTALL) build/pristitrope $(BINDIR)
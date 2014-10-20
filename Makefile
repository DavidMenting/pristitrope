INSTALL = /usr/bin/install
BINDIR = /usr/local/bin

all:
	gcc -Wall -o build/pristitrope pristitrope.c -lwiringPi

install:	all
	$(INSTALL) build/pristitrope $(BINDIR)
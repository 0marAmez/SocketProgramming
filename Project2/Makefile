#------------------------------------------------------------------------------
#  Omar Amezquita 01/24/23 
#  Makefile for Lab2
#  make            makes 
#  clean           removes binary files
#------------------------------------------------------------------------------
CC=gcc
CFLAGS=-Wall -O2

all: bin/myserver bin/myclient

bin/myserver: src/myserver.c
	$(CC) $(CFLAGS) src/myserver.c -o bin/myserver

bin/myclient: src/myclient.c
	$(CC) $(CFLAGS) src/myclient.c -o bin/myclient

clean:
	rm bin/myclient bin/myserver
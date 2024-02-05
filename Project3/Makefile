#------------------------------------------------------------------------------
#  Omar Amezquita 02/10/23 
#  Makefile for Lab3
#  make            makes 
#  clean           removes binary files
#------------------------------------------------------------------------------
CXX=g++
CXXFLAGS=-Wall -O2

all: bin/myserver bin/myclient

bin/myserver: src/myserver.cpp
	$(CXX) $(CXXFLAGS) src/myserver.cpp -o bin/myserver

bin/myclient: src/myclient.cpp
	$(CXX) $(CXXFLAGS) src/myclient.cpp -o bin/myclient

clean:
	rm bin/myclient bin/myserver

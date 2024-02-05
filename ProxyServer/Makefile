#------------------------------------------------------------------------------
#  Omar Amezquita 03/13/23 
#  Makefile for Lab3
#  make            makes 
#  clean           removes binary files
#------------------------------------------------------------------------------
CXX=g++
CXXFLAGS= -pthread -lssl -lcrypto

all: bin/myproxy

bin/myproxy: src/myproxy.cpp
	$(CXX) $(CXXFLAGS) src/myproxy.cpp src/proxy.cpp -o bin/myproxy

clean:
	rm bin/myproxy
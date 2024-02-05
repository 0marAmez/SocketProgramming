# Simple Reliable File Replication
Name: Omar Amezquita
ID: 1876043

## Description
The purpose of this project is to implement redundancy for a file using UDP by extending reliable transfer. It is possible to replicate a local file to multiple servers via the application. The file is reassembled and saved locally by each server based on the packets received from clients. There is a possibility of packets being reordered or lost in transit between the client and server.

## Content
In this set of documents you will find the following directories and files:
    1. /bin - the bin directory will contain all binaries.
    2. /src - the src directory contains the following files:
        a) client.h - Client class library
        b) client.cpp - Client class definition
        c) myclient.cpp - Client main where everything is run
        d) server.h - Server class library
        e) server.cpp - Server class definition
        f) myserver.cpp - Server main where everything is run
    3. /doc - the doc directory contains the documentation for this project and picture containing the graph of the packets vs ack drop.
    4. Makefile - use to compile source files and save the binaries in the /bin directory, and also cleaning.
    5. README - this document.
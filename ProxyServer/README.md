# Simple File Echo
Name: Omar Amezquita
ID: 1876043
## Description
This project aims at developing a proxy HTTP server capable of accepting HTTP requests from clients and converting them into HTTPS requests that will be sent to the web server as part of the HTTPS request process. As well as that, an access control list can be applied to an HTTP request in order to filter it. There is no limitation to the kind of client that can generate the requests, such as using the curl command or a web browser. During a proxy process, a client sends a plain text HTTP request and the proxy then converts it into a HTTPS request and sends it back to the client. The proxy serves as a conduit between the client and the server, converting cleartext HTTP requests from the client to HTTPS requests to the server, and vice versa for the response.

## Content
In this set of documents you will find the following directories and files:
    1. /bin - the bin directory will contain all binaries after using the MAKEFILE.
    2. /src - the src directory contains the following files 
        - proxy.h  -> Class for the proxy server.
        - proxy.cpp -> Class definiton.
        - myproxy.cpp -> Main where everything is run.
    3. /doc - the doc directory contains the documentation for this project.
    4. Makefile - use to compile source files and save the binaries in the /bin directory, and also cleaning.
    5. README - this document.
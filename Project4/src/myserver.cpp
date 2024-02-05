#include "server.h"

int main(int argc, char** argv){
    Server server;
    server.validateArguments(argc,argv);
    server.setValues(argv);
    server.setSocket();
    server.startServer(); 
}
#include "proxy.h"

int main(int argc, char** argv){
    Proxy server;
    server.getArguments(argc, argv);
    //server.test();
    server.startProxy();
}
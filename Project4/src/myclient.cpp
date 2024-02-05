#include "client.h"

int main(int argc, char** argv){
    Client client;
    client.validateArguments(argc,argv);
    client.setValues(argv);
    client.readConfigFile();
    client.startClient();
    //client.test();
}
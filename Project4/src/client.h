#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include<vector>
#include <map>
#include <stdio.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <chrono>
#include <iomanip>
#include <thread>
using namespace std;


#ifndef CLIENT_H
#define CLIENT_H

struct Packet{
       int  connectionFlag; 
       int  sequence;
       int  ack;
       char data[];
};
class Client{
private:
    int                mtu;
    int                window_size;
    int                amountOfservers;
    int                amountOfPackets;
    string             configFile;
    string             input_file;
    string             output_file;
    vector<string>     servers_ip;
    vector<int>        servers_port;
    
public:
    void validateArguments(int argc, char** argv);
    void setValues (char** argv);
    void readConfigFile();
    void startClient();
};

#endif
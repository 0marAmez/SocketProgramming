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
#include <cstdlib> 
#include <ctime>
#include <sys/stat.h>  
#include <algorithm>
#include <chrono>
#include <iomanip>



#define BUFF_SIZE 40960

using namespace std;


#ifndef SERVER_H
#define SERVER_H

class Server{
private:
   struct Packet{
       int  connectionFlag; 
       int  sequence;
       int  ack;
       char data[];
   };
    char                buffer[BUFF_SIZE];
    int                 random;
    int                 socket_fd;
    int                 port;
    int                 drop_rate;
    struct sockaddr_in  client_addr;
    struct sockaddr_in  server_addr;
    bool                dropPacket =   false;
    bool                dropACK =      false;
    socklen_t           len; 
    string              output_path;
    string              root_path;
    vector<Packet*>     file;

    bool duplicateCheck(int sequence);
    bool compareSequence(const Packet* a, const Packet*b);
    void addToVector(Packet*packet);
    void writeToFile();
    void dropPackets();
   
public:
    void validateArguments(int argc, char** argv);
    void setValues (char** argv);
    void setSocket();
    void startServer();
    void Test();

};

#endif
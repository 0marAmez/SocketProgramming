#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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
#include <iostream>
#include <thread>
#include <csignal>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netdb.h>
#include <fcntl.h>

using namespace std;

#define BUFF_SIZE 8096

#ifndef PROXY_H
#define PRXOY_H

class Proxy{
private:
    int             port;
    string          forbiddenSites_file;
    string          log_file;
    map<string,int> forbiddenSites;
public:
    void getArguments(int argc, char** argv);
    void startProxy();
    void test();


};

#endif
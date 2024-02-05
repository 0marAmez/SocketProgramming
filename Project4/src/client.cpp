#include "client.h"
int getAmountOfPackets(string input_file,int mtu){
    FILE *fptr_in;
    int amountOfPackets = 0;
    fptr_in = fopen(input_file.c_str(), "rb");
    if(fptr_in == NULL) {
        cerr<<"**ERROR: Unable to opnen file."<<endl;
        return -1;
        //exit(EXIT_FAILURE);
    }
    fseek(fptr_in, 0L, SEEK_END);
    long int fileSize = ftell(fptr_in);
    if(fileSize%mtu != 0){
         amountOfPackets = fileSize/mtu +1;
    }else{
          amountOfPackets = fileSize/mtu;
    }
    fseek( fptr_in, 0, SEEK_SET );
    fclose(fptr_in);
    return amountOfPackets;
}
void printLog(string ip, int localport,int remotePort ,string type, int sequence,int base, int next,int nextWindow){
        // Get the current time point
        auto now = chrono::system_clock::now();
        // Convert the time point to a time_t object
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        // Convert the time_t object to a struct tm in UTC time
        tm tm_utc = *std::gmtime(&now_time_t);
        // Format the time as an ISO 8601 string
        ostringstream os;
        os << std::put_time(&tm_utc, "%FT%T.") << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000 << "Z";
        string timestap = os.str();
        //2019-02-27T03:19:44.852Z, 9000, 10.1.0.2, 9090, DATA, 7, 5, 8, 15
        cout << timestap << ", "<< localport <<", "<< ip <<", "<< remotePort <<", "<<type<<", "<< sequence <<", "<<base<< ", " <<next <<", "<<nextWindow <<endl;
}
bool checkForChars(string s){
  for (unsigned int i = 0; i < s.length(); i++) {
    if(!(isdigit(s[i]))){
      return true;
    }
  }
  return false;
}
bool checkWindowAck(int ackNum,vector<Packet*>packet_window){
    for(unsigned int i = 0;i<packet_window.size();i++){
        if(ackNum== packet_window[i]->sequence){
            return true;
        }
    }
    return false;
}
void readinputFile(string input_file,string output_file,vector<Packet*>& filePackets,vector<int>&packetsize,int mtu){
    FILE* fptr_in;
    //FILE* fptr_out;
    int sequence = 0;
    int len;
    fptr_in = fopen(input_file.c_str(), "rb");
   // fptr_out = fopen("output.dat", "wb");
    if(fptr_in == NULL) {
        cerr << "ERROR: Unable to open file '"<< input_file <<"'." <<endl; 
        //exit(EXIT_FAILURE);
        return;
    }
    Packet* connectionStart = new Packet[sizeof(*connectionStart) + output_file.length() - 12];
    memcpy( connectionStart->data, output_file.c_str(),output_file.length());
    connectionStart->data[output_file.length()]=0;
    //cout << connectionStart->data <<endl;
    connectionStart->connectionFlag = 0;
    connectionStart->sequence = sequence;
    connectionStart->ack = getAmountOfPackets(input_file,mtu);
    if(connectionStart->ack == -1){
        return;
    }
    sequence++;
    filePackets.push_back(connectionStart);
    packetsize.push_back(output_file.length());

    // filePackets.insert({connectionStart,output_file.length()});

    while(!feof(fptr_in)){
        Packet* packet = new Packet[sizeof(*packet) + mtu - 12];
        packet->sequence = sequence;
        packet->connectionFlag = 1;
        packet->ack = 0;
        len = fread(packet->data, 1, mtu - 12, fptr_in);
        packet->data[len] = 0;
        // if(len<(mtu-12)){
        //     packet->connectionFlag = -100;
        // }
        // fwrite(packet->data,1, len, fptr_out);
        sequence++;
        // filePackets.insert({packet,len});
        filePackets.push_back(packet);
        packetsize.push_back(len);
    }
    filePackets.back()->connectionFlag =-100;
    filePackets.front()->ack = filePackets.size();
    cout << filePackets.size() <<endl;
    fclose(fptr_in);
}
int getLocalPort(int socket){
    // Get the local address and port of the socket
    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    if (getsockname(socket, (struct sockaddr *)&local_addr, &len) == -1) {
        cerr << "Error getting socket name" <<endl;
        return -1;
        // exit(EXIT_FAILURE);        
    }
   // cout << "Local port number: " << ntohs(local_addr.sin_port) <<endl;
    int port =  ntohs(local_addr.sin_port);
    return  port;
}
void resizeWindow(int ackNum,string ip,int port,int localport,vector<Packet*>&packet_window,vector<int>&packet_size,int window_size){
    for(unsigned int i = 0;i<packet_window.size();i++){
        if (packet_window[i]->sequence == ackNum) {
            //delete[] packet_window[i];
            printLog(ip, localport,port ,"ACK",packet_window[i]->sequence, packet_window.front()->sequence,packet_window.front()->sequence+1,packet_window.front()->sequence+window_size-packet_window.size());
            packet_window.erase(packet_window.begin() + i);
            packet_size.erase(packet_size.begin() + i);
            break;
        }
        // else{
        //     //delete[] packet_window[i];
        //     printLog(ip, localport,port ,"ACK",packet_window[i]->sequence, packet_window.front()->sequence,packet_window.front()->sequence+1,packet_window.front()->sequence+window_size-packet_window.size());
        //     packet_window.erase(packet_window.begin() + i);
        //     packet_size.erase(packet_size.begin());
        //     i -=1;
        // }
    }
}
void deletePackets(vector<Packet*> &filesPackets){
    cout << "FILE PACKETS SIZE: " << filesPackets.size() <<endl;
    for(unsigned int i =0; i<filesPackets.size();i++){
        delete[] filesPackets[i];
    }
    filesPackets.clear();
}
void retransmission(struct sockaddr* server_addr, int socket_fd, int& retransmissionCount,vector<Packet*> packet_window,vector<int>packet_size){
    int n = 0;
    for(unsigned int i = 0;i<packet_window.size();i++){
        n = sendto(socket_fd, packet_window[i], sizeof(*packet_window[i])+packet_size[i], 0, (const struct sockaddr *)server_addr, sizeof(*server_addr));
        if(n<0){cerr<<"**ERROR: Window send error."<<endl;
        //exit(EXIT_FAILURE);
        return;
        }
    }
    retransmissionCount++;
}
void printWindow(vector<Packet*>packet_window){
    //cout<< "Current Window of Packets "<<endl;
    for(Packet* i: packet_window)
        cout << i->sequence << " ";
    cout << std::endl;
}
void startConnection(string ip, int port,string input_file,string output_file ,int mtu, int window_size){
    struct sockaddr_in server_addr;
    struct timeval     timeout;
    int                n;
    int                localport;
    int                retransmissionCount = 0;
    int                amountOfPackets;
    bool               PacketAck = false;
    bool               rentransmission = false;
    bool               lastPackets = false;
    bool               timeoutFlag = false;
    bool               serverOffline = false;
    vector<Packet*>    chunkOfData;
    vector<int>        chunckSize;
    // map<Packet*,int>   filePackets;
    vector<Packet*>    packet_window;
    vector<int>        packet_size;


    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_fd<0){ 
        cerr<< "**ERROR: Socket unable to create." <<endl; 
        return;
        //exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str()); 
    server_addr.sin_port = htons(port);

    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        cerr<< "**ERROR: setsockopt failed" <<endl;
        return;
        //exit(EXIT_FAILURE);
    }

    amountOfPackets = getAmountOfPackets(input_file,mtu);
    if(amountOfPackets == -1){
        return;
    }
    // chunkOfData.front()->ack = amountOfPackets;
    readinputFile(input_file,output_file,chunkOfData,chunckSize,mtu);
   //for (auto const& pair : filePackets){
       for(int i =0;i<chunkOfData.size();i++){
        //cout << "Packet sequence sent:" << pair.first->sequence << endl;
        n = sendto(socket_fd,chunkOfData[i], sizeof(*chunkOfData[i]) +chunckSize[i] , 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        localport = getLocalPort(socket_fd);
        if(localport ==-1){
            return;
        } 
        if(n<0){
            cerr<< "**ERROR: File send error."<<endl;
            deletePackets(chunkOfData);
            //exit(EXIT_FAILURE);
            return;
        }
        packet_window.push_back(chunkOfData[i]);
        packet_size.push_back(chunckSize[i]);
        printLog(ip, localport,port ,"DATA",chunkOfData[i]->sequence, packet_window.front()->sequence,packet_window.front()->sequence+1,packet_window.front()->sequence+window_size-packet_window.size());
        amountOfPackets --;
        //cout << "Packets Left: "<<amountOfPackets <<endl;
        if(chunkOfData[i]->connectionFlag==-100){
            lastPackets =true;
        }
        while(packet_window.size()>=window_size || lastPackets){ 
            //printWindow(packet_window);
                Packet* packet = new Packet[sizeof(*packet)];
                n = recvfrom(socket_fd, packet, sizeof(*packet), 0, NULL, NULL);
                if(n<0){
                    if ((errno == EINTR || errno == EWOULDBLOCK)&&serverOffline==true){
                        cerr<<"**ERROR: Cannot detect server IP port."<<endl;
                        deletePackets(chunkOfData);
                        //exit(EXIT_FAILURE);
                        return;
                    }else if ((errno == EINTR || errno == EWOULDBLOCK)&&rentransmission==true){
                        cout<< "Packet loss detected! Timeout!" <<endl;
                        retransmission((struct sockaddr*)&server_addr, socket_fd ,retransmissionCount,packet_window,packet_size);
                        timeoutFlag = true;
                    }else if ((errno == EINTR || errno == EWOULDBLOCK)&&rentransmission==false){
                        cerr<< "ERROR: Cannot detect server." <<endl;
                        deletePackets(chunkOfData);
                        //exit(EXIT_FAILURE);
                        return;
                    }else{
                        cerr<<"**ERROR: Received error occurred. "<<endl;
                        deletePackets(chunkOfData);
                        //exit(EXIT_FAILURE);
                        return;
                    }
                }
                if(timeoutFlag== false){
                    if(packet->connectionFlag == 1 || serverOffline == true){
                        //cout << "Setting socket 10s timeout" <<endl;
                        timeout.tv_sec = 10;
                        timeout.tv_usec = 0;
                        if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
                            cerr<<"**ERROR: setsockopt failed"<<endl;
                            //exit(EXIT_FAILURE);
                            deletePackets(chunkOfData);
                            return;
                        }
                        rentransmission = true;
                    }
                    //cout << "Received ACK: " <<  packet->ack<< endl;
                    if(checkWindowAck(packet->ack,packet_window)==true){
                        resizeWindow(packet->ack,ip,port,localport,packet_window,packet_size,window_size);
                        retransmissionCount = 0;
                    }else{
                        cout<< "Packet loss detected!  Not in window" <<endl;
                        retransmission((struct sockaddr*)&server_addr, socket_fd ,retransmissionCount,packet_window,packet_size);
                    }
                    if(packet_window.size()==0){
                        lastPackets=false;
                    }
                }
                if( retransmissionCount>= 10){
                    cerr<< "**ERROR: Reached max re-transmission limit."<<endl;
                    //exit(EXIT_FAILURE);
                    deletePackets(chunkOfData);
                    return;
                }
                if(retransmissionCount == 6){ // Possible server offline
                    serverOffline = true;
                    timeout.tv_sec = 30;
                    timeout.tv_usec = 0;
                    if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
                        cerr<<"**ERROR: setsockopt failed"<<endl;
                        //exit(EXIT_FAILURE);
                        deletePackets(chunkOfData);
                        return;
                }
                rentransmission = false;
            }
            timeoutFlag = false;   
            delete []packet;
        } 

    }
    close(socket_fd);
    deletePackets(chunkOfData);
}
void Client::validateArguments(int argc, char** argv){
    ifstream file;
    if(argc < 7 || argc>7){
        cerr<< "ERROR: Please, enter no more or less than 7 arguments."<<endl;
        exit(EXIT_FAILURE);
    }
    if(checkForChars(argv[1])==true || checkForChars(argv[3]) == true || checkForChars(argv[4]) == true){
        cerr<< "ERROR: Please, make sure to enter the right format for your the MTU, Window size, and the number of servers."<<endl;
        exit(EXIT_FAILURE);
    }
    file.open(argv[2]);
    if(!file.good()){
        cerr<< "ERROR: File '" << argv[2] << "' does not exist."<<endl;
        exit(EXIT_FAILURE);
    }
    ifstream file_1(argv[5]);
    if(!file_1.good()){
        cerr<< "ERROR: File '" << argv[5] << "' does not exist."<<endl;
        exit(EXIT_FAILURE);
    }

}
void Client::setValues (char** argv){
    amountOfservers = atoi(argv[1]);
    configFile = argv[2];
    mtu = atoi(argv[3]);
    if(mtu<12 || mtu>32000){
        cerr<< "**ERROR: Required minimum MTU is 13 bytes and cannot be greater 31988 bytes."<<endl;
        exit(EXIT_FAILURE);
    }
    window_size = atoi (argv[4]);
    if(window_size<=0 || window_size>1024){
        cerr<< "**ERROR: Invalid window size. Window size should be greater 0 and less than 1024 or equal"<<endl;
        exit(EXIT_FAILURE);
    }
    input_file =  argv[5];
    output_file = argv[6];
}
void Client::readConfigFile(){
    ifstream infile(configFile);
    string line;
    string ip;
    string port;
    string server_ip;
    struct in_addr addr;
    unsigned int position;
    int server_port = 0;
    int count = 0;
    int serverInfo = 0;
    while (getline(infile, line)) { 
        count++;
        if(line[0]!='#'){
            serverInfo++;
            position = line.find(' ');
            if(position!=string::npos){
                ip = line.substr(0, position);
                port = line.substr(position+1);
                if(inet_pton(AF_INET, ip.c_str(), &addr)!=1){
                    cerr<< "**Error: Invalid IPv4 address '"<<ip<<"' on line " << count<< "." << endl; 
                    exit(EXIT_FAILURE); 
                }
                if(checkForChars(port) == true){
                    cerr <<"**Error: Invalid port number '" << port <<"' on line " << count<< "." << endl;
                    exit(EXIT_FAILURE); 
                }
                if(serverInfo>amountOfservers){
                    cerr<< "**ERROR: More than " << amountOfservers << " server information found in this document." <<endl;
                    exit(EXIT_FAILURE);
                }
                server_port = atoi(port.c_str());
                server_ip = ip;
                servers_ip.push_back(server_ip);
                servers_port.push_back(server_port);
            }
        }
    }
    if (serverInfo<amountOfservers){
        cerr<< "**ERROR: Less than " << amountOfservers << " server information found in this document." <<endl;
        exit(EXIT_FAILURE);
    }
    infile.close();
}
void Client::startClient(){
    startConnection("127.0.0.1",9090,input_file,output_file,mtu,window_size);
    // vector<thread> threads;
    // for(unsigned int i =0; i < servers_port.size(); i++){
    //     threads.emplace_back(startConnection, servers_ip[i], servers_port[i],input_file,output_file,mtu,window_size);
    // }
    // for (auto& thread : threads) {
    //     thread.join();
    // }

}
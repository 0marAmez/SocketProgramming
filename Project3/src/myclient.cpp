//Program written by Omar Amezquita 02/06/23
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <cstdio>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
 
using namespace std;

struct Packet {
    int seq_num;
    int ack_num;
    char data[];
};
// void retransmission(const vector<Packet*> &vec,const vector<int> &vec2,int socket,struct sockaddr_in *server_addr);
void printWindow(const vector<Packet*> &vec);
//check if ACKS is inside the sent Window
bool ackWindowCheck(const vector<Packet*> &vec,int ackNum);
// Get's rid of packet that we
void deletedACKPackets(vector<Packet*> &vec,vector<int> &vec2,int ackNum);
//bool isHere(const vector<Packet*> &vec);
bool checkForChars(string s);
// void printLog
void printLog(const vector<Packet*> &vec);

int main(int argc, char** argv){
//Variables used to save arguments
    string server_ip, in_file_path,out_file_path;
    int server_port,mtu,window_size,n;
    n = 0;
    struct in_addr addr;
// Socket Variables
    int socket_fd;
    struct sockaddr_in server_addr;
// FILE I/O
    FILE* fptr_in;
// ARGUMENT VALIDATION
    // Check for the right amount of arguments
    if(argc < 7 || argc>7){perror("**Error: Make sure to only enter 6 arguments.");return -1;}
    if(checkForChars(argv[2]) == true|| checkForChars(argv[3])== true || checkForChars(argv[4])== true ){perror("**Error: Invalid argument syntax in either mtu, port, or window size.");return -1;}
    if(inet_pton(AF_INET, argv[1], &addr)!=1){perror("**ERROR:Invalid Ipv4 address \n");return -1;}
    
    server_ip = argv[1];
    server_port = atoi(argv[2]);
    mtu = atoi(argv[3]);
    window_size = atoi(argv[4]);
    in_file_path = argv[5];
    out_file_path = argv[6];
    
    if(mtu<=10){perror("**ERROR:Required minimum MTU is 10");return -1;}
    if(window_size<=0){perror("**ERROR:Please enter a valid size for the window.");return -1;};
    if (out_file_path.find('/')<out_file_path.length()){
        if(out_file_path[0]!='/'){
            perror("**ERROR:Please enter a valid format for the output file. Make sure it starts with '/' .");
            return -1;
        }
 
    }



//Creates Socket and checks for error
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_fd<0){ perror("**ERROR: Socket unable to create."); return -1;}
// Specify server information
    memset(&server_addr, 0, sizeof(server_addr)); //  Zero out the variable
    server_addr.sin_family = AF_INET; // Ipv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str()); // Any IP assign
    server_addr.sin_port = htons(server_port); // Port number

// Open output.dat file for writing
    fptr_in = fopen(in_file_path.c_str(), "rb");
    if(fptr_in == NULL) {perror("**ERROR: Unable to opnen file.");return -1;}



// Get File Size
    int amountOfpackets = 0;
    fseek(fptr_in, 0L, SEEK_END);
    long int fileSize = ftell(fptr_in);
    // cout<< "Size of the file is " << fileSize<<endl;
    if(fileSize%mtu != 0){
         amountOfpackets = fileSize/mtu +1;
    }else{
          amountOfpackets = fileSize/mtu;
    }
    // cout<< "Packets to the send " << amountOfpackets<<endl;
    fseek( fptr_in, 0, SEEK_SET );



   struct timeval timeout;
    // Set the timeout value
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
   if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        perror("**ERROR: setsockopt failed");
        return -1;
    }

// Client Loop
    int seq_num = -1;
    int len =0;
    int packetLen = 0;
    int restransmissionCount = 0;
    vector<Packet*> window_Packets;
    vector<int> packetSize;
    std::chrono::time_point<std::chrono::steady_clock> start;
    bool start_initialized = false;
    bool last_packets = false;
    Packet *packet;
    Packet *temp;
    bool packetAcked = false;


    while (!feof(fptr_in)){
        // cout<< "Packet sequence sent:" << seq_num << endl;
        if(seq_num==-1){
            // cout << "PACKET PATH SENT"<<endl;
            packet = new Packet[sizeof(*packet) +out_file_path.size()];
            temp = new Packet[sizeof(*packet) +out_file_path.size()];
            memcpy( packet->data, out_file_path.c_str(),out_file_path.size());
            packet->seq_num =temp->seq_num= seq_num;
            packet->ack_num=temp->ack_num = -1;
            seq_num++;
            sendto(socket_fd, packet, sizeof(*packet) + out_file_path.length(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            if(n<0){perror("**ERROR: File send error.");return -1;}
            packetLen - out_file_path.length();
        }
        else{
            packet = new Packet[sizeof(*packet) + mtu - 8];
            temp = new Packet[sizeof(*packet) + mtu - 8];
            packet->seq_num =temp->seq_num= seq_num;
            packet->ack_num=temp->ack_num = 0;
            seq_num++;
            len = fread(packet->data, 1, mtu - 8, fptr_in);
            if(len<(mtu-8)){
                packet->ack_num = -100;
            }
            n = sendto(socket_fd, packet, sizeof(*packet) + len, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
            if(n<0){perror("**ERROR: File send error.");return -1;}
            packetLen = len;
        }
        
        memcpy( temp->data, packet->data,mtu-8); // copies the data required from the ptr
        window_Packets.push_back(temp);
        packetSize.push_back(packetLen);
        amountOfpackets--;
        if(amountOfpackets == 0){
            last_packets =true;
        }

        while(window_Packets.size()>=window_size|| last_packets==true){
            printLog(window_Packets);
        //   printWindow(window_Packets);
          n = recvfrom(socket_fd, packet, sizeof(*packet), 0, NULL, NULL);
          if(n<0){
                if ((errno == EINTR || errno == EWOULDBLOCK)&&start_initialized==true){
                 cout<< "Packet loss detected!" <<endl;
                 for(unsigned int i = 0;i<window_Packets.size();i++){
                     n = sendto(socket_fd, window_Packets[i], sizeof(*window_Packets[i])+packetSize[i], 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
                    if(n<0){perror("**ERROR: File send error.");exit(EXIT_FAILURE);}
                 }
                restransmissionCount++; 
                }else if((errno == EINTR || errno == EWOULDBLOCK)&&start_initialized==false){
                    perror("ERROR: Cannot detect server.");
                    return -1;

                }else{
                    perror("**ERROR: Received error occurred. ");
                    return -1;
                } 
          }
          if(packet->ack_num == -1){
                  timeout.tv_sec = 10;
                  timeout.tv_usec = 0;
                  if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
                    perror("**ERROR: setsockopt failed");
                    return -1;
                 }
                 start_initialized = true;
          }
          if(ackWindowCheck(window_Packets,packet->ack_num)==true){
            //   cout << "ACK PACKET: "<<packet->ack_num << endl;
              deletedACKPackets(window_Packets,packetSize,packet->ack_num);
              restransmissionCount = 0;
              packetAcked = false;
          }
          else{
                 cout<< "Packet loss detected!" <<endl;
                 for(unsigned int i = 0;i<window_Packets.size();i++){
                     n = sendto(socket_fd, window_Packets[i], sizeof(*window_Packets[i])+packetSize[i], 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
                    if(n<0){perror("**ERROR: File send error.");exit(EXIT_FAILURE);}
                 }
                restransmissionCount++; 

          }
          if(restransmissionCount >= 5){
                perror("**ERROR: Reached max re-transmission limit.");
                return -1;
           }

          if(window_Packets.size() == 0||packetAcked == true){  packetAcked = false; break;}
        }
        delete packet;
    }

// cout << "Packets left: " <<amountOfpackets << endl;
// CLOSE SOCKET AND FILE       
    fclose(fptr_in);
    close(socket_fd);	

	return 0;
}
bool checkForChars(string s){
  for (unsigned int i = 0; i < s.length(); i++) {
    if(!(isdigit(s[i]))){
      return true;
    }
  }
  return false;
}
void printWindow(const vector<Packet*> &vec){
    cout<< "Current Window of Packets "<<endl;
    for(Packet* i: vec)
        std::cout << i->seq_num << " ";
    std::cout << std::endl;
}
bool ackWindowCheck(const vector<Packet*> &vec, int ackNum){
    for(unsigned int i = 0;i<vec.size();i++){
        if(ackNum== vec[i]->seq_num){
            return true;
        }
    }
    return false;
}
void deletedACKPackets(vector<Packet*> &vec,vector<int> &vec2,int ackNum){
    for (unsigned int i = 0; i < vec.size(); i++) {
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
        if (vec[i]->seq_num == ackNum) {
            cout<< timestap << ", ACK, "<< vec[i]->seq_num << ", " <<vec.front()->seq_num << ", "<< vec[i]->seq_num+1<< ", "<< vec[i]->seq_num+vec.size() <<endl;
            delete vec[i];
            vec.erase(vec.begin());
            vec2.erase(vec2.begin());
            break;
        } 
        else {
            cout<< timestap << ", ACK, "<< vec[i]->seq_num << ", " <<vec.front()->seq_num << ", "<< vec[i]->seq_num+1<< ", "<< vec[i]->seq_num+vec.size() <<endl;
            delete vec[i];
            vec.erase(vec.begin() + i);
            vec2.erase(vec2.begin());
            i -=1;
        }
    }


}
void printLog(const vector<Packet*> &vec){

    for(unsigned int i = 0;i<vec.size();i++){
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

        cout<< timestap << ", DATA, "<< vec[i]->seq_num << ", " <<vec.front()->seq_num << ", "<< vec[i]->seq_num+1<< ", "<< vec[i]->seq_num+vec.size() <<endl;
    }
    
}
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
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h> 
#include <errno.h>
#include <chrono>
#include <iomanip>

using namespace std;



#define BUFF_SIZE 4096
// Flags
bool dropPacket = false;
bool dropACK = false;
//ACK Temp
int tempACK;
// FILE I/O
FILE* fptr_out;
// Packet Structure
struct Packet {
    int seq_num;
    int ack_num;
    char data[];
};
bool checkForChars(string s); 
void writeToFile(string file,vector<Packet*> &vec);
bool inside(const vector<Packet*> &vec,int packetSeq);
void addToVector(vector<Packet*> &vec, Packet * temp);
void printWindow(const vector<Packet*> &vec);
void dropPackets();
void createDirectory(string path);
int main(int argc, char** argv){
    char buffer[BUFF_SIZE];
    string output_file;
// Arguments variables
    int server_port,probTodrop_pck;
//Socket variables
    int socket_fd,n,rcv_n;
    struct sockaddr_in server_addr, client_addr;
    socklen_t                               len;
// Random number
    srand(time(0));
    int randomNumber = 0;
// Check for the right amount of arguments
    if(argc < 3 || argc>3){perror("**Error: Make sure to enter 2 arguments.");return -1;}
//Check for argument validation
    if(checkForChars(argv[1]) == true|| checkForChars(argv[2])== true){perror("**Error: Invalid argument syntax in either mtu, port, or window size.");return -1;}
//Assign Variables
    server_port =  atoi(argv[1]); 
    probTodrop_pck = atoi(argv[2]);

    if(probTodrop_pck<0 || probTodrop_pck>100){perror("**Error: Invalid argument, please enter a valid drop rate.");return -1;}

//Creates Socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
// Checks for error in the socket creation
    if(socket_fd<0){perror("**ERROR: Socket unable to create. \n"); return -1;}
//bzero() function places n zero-valued bytes in the area pointed to by s
//zero out server_addr
    bzero(&server_addr, sizeof(server_addr));
    // Specify server information
    server_addr.sin_family = AF_INET; // Ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Any IP assign
    server_addr.sin_port = htons(server_port); // Port number
// Binds a unique local name to the socket with descriptor socket and checks for errors 
    if(bind(socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))<0){perror("**ERROR: Binding failed."); return -1;}

// Server Loop
    cout<<"Server Ready ....." << endl;
    vector<Packet*> file_Packets;
    int receivedWindow = -1;
    int recevivedACKS= -2;
    // bool packetMissing = false;
    Packet *ptr;
    // vector<int> packet_size;
    // int acktemp = 0;
    while(true){
      randomNumber =  rand() % 100 + 1;
      len = sizeof(client_addr);
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
      if(randomNumber<=probTodrop_pck){
          dropPackets();
      }else{
          dropPacket = false;
          dropACK = false;
      }
      if(dropPacket==false){
        rcv_n = recvfrom(socket_fd, buffer, BUFF_SIZE, 0,(struct sockaddr *) &client_addr, &len);
        if(rcv_n<0){ perror("**ERROR: Received error occurred."); exit(EXIT_FAILURE);} // Check for received error
        Packet *temp = new Packet[sizeof(*temp) + rcv_n - 8];
        ptr = (Packet*) buffer;
        if(ptr->seq_num==-1){
            recevivedACKS = -2;
            ptr->data[rcv_n-8] = 0;
            output_file = ptr->data;
            // cout<< "PATH: "<< output_file<<endl;
        }else{
            temp->seq_num=(ptr)->seq_num;
            ptr->data[rcv_n-8] = 0; // clears the data buffer 
            temp->ack_num = rcv_n-8;
            memcpy(temp->data,ptr->data,rcv_n-8);
            addToVector(file_Packets,temp);
       }
        recevivedACKS++;
        if(ptr->ack_num == -100){
            writeToFile(output_file,file_Packets);
        }
        receivedWindow = ptr->seq_num;
        cout<< timestap << ", DATA, "<< ptr->seq_num <<endl;
        
      }else{
        if(recevivedACKS!= -2){
          cout<< timestap << ", DROP DATA, "<< receivedWindow+1 <<endl;
        }
      }
       
       Packet * packet = new Packet[sizeof(*packet)]; 
       packet->ack_num = receivedWindow;
       if(packet->ack_num == -1){
           dropACK = false;
       }

       if(dropACK == false){ 
           if(recevivedACKS!=-2){
            n = sendto(socket_fd, packet, sizeof(*packet), 0, (const struct sockaddr *) &client_addr, len);
            if(n<0){perror("**ERROR: Sent error occurred."); exit(EXIT_FAILURE);}
            cout<< timestap << ", ACK, "<< packet->ack_num <<endl;
           }
        }else{
            if(recevivedACKS !=-2){
                cout<< timestap << ", DROP ACK, "<< packet->ack_num <<endl;
            }
        }

        delete packet;
    }

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
void writeToFile(string path,vector<Packet*> &vec){
    // cout<< path <<endl;

    if (path.front() == '/') {
        string temp = path;
        temp.erase(0, 1);
        createDirectory(temp);
        string filename = path.substr(path.find_last_of("/") + 1);
        // cout << filename <<endl;
        fptr_out = fopen(filename.c_str(), "wb");
        if(fptr_out == NULL) {perror("**ERROR: Unable to opnen file.");exit(EXIT_FAILURE);}
    }else{
        fptr_out = fopen(path.c_str(), "wb");
        if(fptr_out == NULL) {perror("**ERROR: Unable to opnen file.");exit(EXIT_FAILURE);}
    }

    for (unsigned int i = 0; i<vec.size();i++){
        fwrite(vec[i]->data,1, vec[i]->ack_num, fptr_out);
        delete vec[i];
    }
    vec.clear();
    // printWindow(vec);
    // cout << vec.size()<<endl;
    fclose(fptr_out);
}
bool inside(const vector<Packet*> &vec,int packetSeq){
    for(Packet* i: vec){
        if(i->seq_num== packetSeq){
            return true;
        }
    }
    return false;
}
bool compareBySequence(const Packet* a, const Packet*b){
    return a->seq_num<b->seq_num;
}
void addToVector(vector<Packet*> &vec, Packet * temp){
    if (vec.empty()){
        vec.push_back(temp);
    }else{
        if(inside(vec,temp->seq_num)==false){
            vec.push_back(temp);
            std::sort(vec.begin(), vec.end(), compareBySequence);
           // printWindow(vec);
        }
    }
}

void printWindow(const vector<Packet*> &vec){
    cout<< "Current Window of Packets "<<endl;
    for(Packet* i: vec)
        std::cout << i->seq_num << " ";
    std::cout << std::endl;
}
void dropPackets(){
    int typeofdrop = (rand() % 2) + 1;
    if(typeofdrop== 1){
        dropPacket = true;
       dropACK = false;
    }else{
        dropACK = true;
       dropPacket = false;
    }
}
void createDirectory(string path){
    unsigned int position = 0;
    string token;
    while ((position = path.find('/')) != string::npos) {
        token = path.substr(0, position);
        if (mkdir(token.c_str(), 0777) == -1 && errno != EEXIST) {
            perror("Error creating directory: " );
            exit(EXIT_FAILURE);
        }
        chdir(token.c_str());
        path.erase(0, position + 1);
    }
} 
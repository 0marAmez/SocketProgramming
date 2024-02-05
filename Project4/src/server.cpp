#include "server.h"

bool checkForChars(string s){
  for (unsigned int i = 0; i < s.length(); i++) {
    if(!(isdigit(s[i]))){
      return true;
    }
  }
  return false;
}
void printLog(int localport,string remoteIp, int rport,string type, int sequence){
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
        //2019-02-27T03:19:44.852Z, 9090, 10.1.1.1, 43000, ACK, 5
        cout << timestap << ", "<< localport <<", "<< remoteIp <<", "<< rport <<", "<<type<<", "<< sequence <<endl;
}
void createDirectory(string root_directory,string output_path){
    // cout << "Create Directory " <<endl; 
    chdir(root_directory.c_str());
    int position = 0;
    string token;
    while ((position = output_path.find('/')) != string::npos){
        token = output_path.substr(0, position);
        //cout<< token << endl;
        if (mkdir(token.c_str(), 0777) == -1 && errno != EEXIST) {
            perror("Error creating directory: " );
            exit(EXIT_FAILURE);
        }
        chdir(token.c_str());
        output_path.erase(0, position + 1);
        //cout<<path<<endl;
    }
} 
bool Server::duplicateCheck(int sequence){
    for(Packet* i: file){
        if(i->sequence== sequence){
            return true;
        }
    }
    return false;
}
bool Server::compareSequence(const Packet* a, const Packet*b){
    return a->sequence<b->sequence;
}
void Server:: addToVector (Packet*packet){
     if(file.empty()){
       file.push_back(packet);
     }else{
       if(duplicateCheck(packet->sequence)==false){
         file.push_back(packet);
         sort(file.begin(), file.end(), [this](Packet* a, Packet* b) { return this->compareSequence(a, b); });
       }
     }
}
void Server:: writeToFile(){
    FILE* fptr_out;
    //cout << "WRITETOFILE:"<< file.size() <<endl;
    string full_path = root_path+"/"+output_path;
    createDirectory(root_path,output_path);
    string filename = full_path.substr(full_path.find_last_of("/") + 1);
    fptr_out = fopen(filename.c_str(), "wb");
    if(fptr_out == NULL) {cerr<<"**ERROR: Unable to open file."<<endl;exit(EXIT_FAILURE);}
    for (int i = 0 ; i <file.size();i++) {
      file[i]->data[file[i]->ack]=0;
      fwrite(file[i]->data,1, file[i]->ack, fptr_out);
      delete[] file[i];
    }
    file.clear();
    fclose(fptr_out);
}
void Server::validateArguments(int argc, char** argv){
    if(argc < 4 || argc>4){
        cerr<< "**ERROR: Please enter the right amount of arguments 3." << endl;
        exit(EXIT_FAILURE);
    }
    if(checkForChars(argv[1])==true || checkForChars(argv[2]) == true){
        cerr<< "ERROR: Please, make sure to enter the right format for your the Port, and Drop Rate."<<endl;
        exit(EXIT_FAILURE);
    }
    int temp = atoi(argv[2]);
    if(temp <0 || temp>100){
        cerr<< "ERROR: Please, make sure that the value of drop rate is between 0-100."<<endl;
        exit(EXIT_FAILURE);
    }
    ifstream file_1(argv[3]);
    string s = argv[3];
    if(!file_1.good()){
       mkdir(s.c_str(), 0777);
    }
}
void Server::setValues (char** argv){
    port = atoi(argv[1]);
    drop_rate = atoi(argv[2]);
    root_path = argv[3];
}
void Server::setSocket(){
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_fd<0){
      cerr<< "**ERROR: Socket unable to create." <<endl;
      exit(EXIT_FAILURE);
    }
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // Ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Any IP assign
    server_addr.sin_port = htons(port); // Port number
    if(bind(socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))<0){
      cerr<< "**ERROR: Binding failed." << endl; 
      exit(EXIT_FAILURE);
    }

}
void Server:: dropPackets(){
  srand(time(nullptr));
  int num = 0;
  random =  rand() % 101;
  if(random<=drop_rate){
      num = (rand() % 2) + 1;
      if(num == 1){
        dropPacket = true;
        dropACK = false;
      }else{
        dropACK = true;
        dropPacket = false;
    }
  }else{
        dropACK =false;
        dropPacket =false;
    }
}

void Server::startServer(){
    Packet *ptr;
    int     n_bytes;
    int     num = 0;
    int     communicationFlag;
    int     receivedSequence = 0;
    int     amountofPackets;
    int     remotePort;
    int     count = 0;
    bool    connection = false;
    bool    start = false;
    bool    lastReceived = false;
    char    ip_str[INET_ADDRSTRLEN];
    string  ipTemp;
    cout<<"Server Ready ....." << endl;
    while(true){
      len = sizeof(client_addr);
      dropPackets();
      if(dropPacket == false){
       // cout << "FILE SIZE PACKETS: " <<file.size() <<endl;
          n_bytes = recvfrom(socket_fd, buffer, BUFF_SIZE, 0,(struct sockaddr *) &client_addr, &len);
          if(n_bytes<0){ cerr << "**ERROR: Received error occurred."<<endl; exit(EXIT_FAILURE);}
          ptr = (Packet*) buffer;
          inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
          string remoteip(ip_str);
          ipTemp = remoteip;
          remotePort = ntohs(client_addr.sin_port);
          printLog(port,ipTemp, remotePort,"DATA", ptr->sequence);
          //cout <<"PACKET RECEIVED: " <<ptr->sequence <<endl;
          Packet *temp = new Packet[sizeof(*temp) + n_bytes - 12];
          if(ptr->connectionFlag == 0){
            communicationFlag = ptr->connectionFlag;
            amountofPackets = ptr->ack;
            ptr->data[n_bytes-12] = 0;
            output_path = ptr->data;
            connection = true;
            start = true;
            receivedSequence = ptr->sequence;
            // cout << "Output file: " << output_path << endl;
          }else{
              communicationFlag = ptr->connectionFlag; // might used for dropping packets
              if(communicationFlag == -100){
                //cout <<"LAST PACKET RECEUVED" <<endl;
                lastReceived = true;
              }
              temp->sequence=ptr->sequence;
              receivedSequence = ptr->sequence;
              temp->ack = n_bytes-12;
              ptr->data[n_bytes-12] = 0; 
              memcpy(temp->data,ptr->data,n_bytes-12);
              temp->data[n_bytes-12] =0;
              addToVector(temp);
          }
          // if(ptr->connectionFlag == -100){
          //   communicationFlag = ptr->connectionFlag;
          //   writeToFile();
          // }
      }else{
       if(start == true){
         printLog(port,ipTemp, remotePort,"DROP DATA", receivedSequence+1);
          //cout << "Packet drop! sequence: " << receivedSequence+1 <<endl;
        }
     }

      Packet * packet = new Packet[sizeof(*packet)];
      if(connection){
        packet->connectionFlag = 1;
        dropACK = false;
        connection = false;
      }else{
        packet->connectionFlag = 2;
      }
      packet->ack = receivedSequence; // might need change
      if(dropACK == false&& start == true){
        //cout <<"PACKET SENT: " <<packet->ack <<endl;
        printLog(port,ipTemp, remotePort,"ACK", packet->ack);
        n_bytes = sendto(socket_fd, packet, sizeof(*packet), 0, (const struct sockaddr *) &client_addr, len);
        if(n_bytes<0){cerr<<"**ERROR: Sent error occurred."<<endl; exit(EXIT_FAILURE);}
        if(lastReceived&&file.size()==amountofPackets-1){
          //cout <<"WRITE FILE" <<endl;
          lastReceived = false; start = false; writeToFile();}
     }
      else{
        if(start == true){
          printLog(port,ipTemp, remotePort,"DROP ACK", packet->ack);
          //cout << "ACK drop! sequence: " << packet->ack <<endl;
        }
      }
      delete[] packet;
    }
    close(socket_fd);   
}

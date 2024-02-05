// Program written by Omar Amezquita 01/24/23
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <ctype.h>

// #define BUFF_SIZE 1024


void error_exit(char *msg) {
  perror(msg);
  exit(-1);
}

int checkForChars(char *s) {
  for (int i = 0; i < strlen(s); i++) {
    if(!(isdigit(s[i]))){
      return -1;
    }
  }
  return 1;
}

int main(int argc,char* argv[]){
    //Variables used to save arguments
    char* server_ip;
    char* in_file_path;
    char* out_file_path;
   
    int server_port,mtu,socket_fd,n;
    struct sockaddr_in server_addr;
    // Check if IP address is valid
    struct in_addr addr;
    socklen_t servlen;

    // time checking
    struct timeval timeout;

    // FILE I/O
    FILE *fptr_in;
    FILE* fptr_out;
    size_t bytes_Read;

    // Check for the right amount of arguments
    if(argc < 6 || argc>6){
      error_exit("**Error: Make sure to only enter 5 arguments.");
    }
    //Checks for valid input
    if(checkForChars(argv[2]) == -1 || checkForChars(argv[3])== -1){
        error_exit("**Error: Invalid argument syntax.");
    }
    
    // initialize variables
    server_ip = argv[1];
    server_port = atoi(argv[2]);
    mtu = atoi(argv[3]);
    in_file_path = argv[4];
    out_file_path = argv[5];
    
  //ARGUMENT VALIDATION
    //Checks if IP is a valid IPv4
    if(inet_pton(AF_INET, server_ip, &addr)!=1){
        error_exit("**ERROR:Invalid Ipv4 address \n");
    }
     //Checks for valid input for the Port and MTU
    if(mtu == 0 || server_port== 0){
       error_exit("**ERROR: Invalid argument input for mtu or server port.");
    }

    //Creates Socket and checks for error
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_fd<0){
      error_exit("**ERROR: Socket unable to create.");
    }

    // Creates sendLine and rcvLine buffer
    unsigned char sendLine[mtu];
    unsigned char recvLine[mtu+1];

    //bzero() function places n zero-valued bytes in the area pointed to by s
    //zero out server_addr
    bzero(&server_addr, sizeof(server_addr));

    // Specify server information
    server_addr.sin_family = AF_INET; // Ipv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // Any IP assign
    server_addr.sin_port = htons(server_port); // Port number

    // Open output.dat file for writing
    fptr_in = fopen(in_file_path, "rb");
    fptr_out = fopen(out_file_path, "wb");
    if(fptr_in == NULL || fptr_out == NULL) {
      error_exit("**ERROR: File error creation. \n");
    }

    // size of server adddress
    servlen = sizeof(server_addr);

    // Set the timeout value
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;

    if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        error_exit("**ERROR: setsockopt failed");
    }

    //Loop that print output and input
    //fgets(sendLine, mtu, fptr_in) != NULL
    //fread(sendLine,1,mtu,fptr_in)>0  
    while (!feof(fptr_in)){
        bytes_Read = fread(sendLine,1,mtu,fptr_in);
        n = sendto(socket_fd, sendLine, bytes_Read, 0, (struct sockaddr*)&server_addr, servlen);
        if(n<0){
          error_exit("**ERROR: Sent error occurred. ");
        }
        n = recvfrom(socket_fd, recvLine, mtu, 0, NULL, NULL);
        if(n<0){
          if(errno == EINTR || errno == EWOULDBLOCK){ // difference?
            error_exit("**ERROR: Timeout error occurred.");
          }
          error_exit("**ERROR: Received error occurred. ");
        }
        recvLine[n] = 0;
        fwrite(recvLine,1, bytes_Read, fptr_out);
       //fputs( recvLine, fptr_out);
    }


	  // Close file
    fclose(fptr_in);
    fclose(fptr_out);
    // Close the socket
	  close(socket_fd);
    
    return 0;
}

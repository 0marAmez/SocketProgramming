// Program written by Omar Amezquita 01/24/23
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFF_SIZE 4096

void error_exit(char *msg) {
  perror(msg);
  exit(1);
}
int checkForChars(char *s) {
  for (int i = 0; i < strlen(s); i++) {
    if(!(isdigit(s[i]))){
      return -1;
    }
  }
  return 1;
}
//file descriptor
int main(int argc,char* argv[]){
    char buffer[BUFF_SIZE];
    int server_port,socket_fd,n,rcv_n;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len;
    
    // Check for the right amount of arguments
    if(argc < 2 || argc>2){
      error_exit("**Error: Make sure to only enter 1  or at least 1 argument.");
    }
    //Checks for valid input
    if(checkForChars(argv[1]) == -1){
        error_exit("**Error: Invalid argument syntax.");
    }
    server_port =  atoi(argv[1]); // Some way to check the validity of the port?


    //Creates Socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    // Checks for error in the socket creation
    if(socket_fd<0){
      error_exit("**ERROR: Socket unable to create. \n");
    }
    //bzero() function places n zero-valued bytes in the area pointed to by s
    //zero out server_addr
    bzero(&server_addr, sizeof(server_addr));
    // Specify server information
    server_addr.sin_family = AF_INET; // Ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Any IP assign
    server_addr.sin_port = htons(server_port); // Port number
    
    // Binds a unique local name to the socket with descriptor socket and checks for errors 
    if(bind(socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))<0){
      error_exit("**ERROR: Binding failed.");
    }
    // Loop waits for request
    printf("Sever ready.....\n");
    while(1){
      len = sizeof(client_addr);
      rcv_n = recvfrom(socket_fd, buffer, BUFF_SIZE, 0,(struct sockaddr *) &client_addr, &len);
      if(rcv_n<0){
        error_exit("**ERROR: Received error occurred.");
      }
      buffer[rcv_n] = 0;
      //printf("Received: %s\n", buffer);
      n = sendto(socket_fd, buffer, rcv_n, 0, (const struct sockaddr *) &client_addr, len);
      if(n<0){
        error_exit("**ERROR: Sent error occurred.");
      }
  
    }

    close(socket_fd);

    return 0;
}
// Program written by Omar Amezquita 01/20/23
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 8192

int main(int argc,char* argv[]){

    // Variables used to create Socket
    int my_socket,is_connected,n,header_passed,bytes_received,option_h,content_length;
    option_h = 0;
    // Buffer array
    char data_buffer[BUFF_SIZE];
    // Content string
    char *header; 
    // String used for the HTTP request
    char request_template[BUFF_SIZE];
    // Structure used for the server information
    struct sockaddr_in server_address;
    // Check if IP address is valid
    struct in_addr addr;

    // FILE I/O
    FILE *fptr;

    // Creates Socket
    // AF_INET = IPv4, SOCK_STREAM = TCP
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    //Variables use for arguments
    int port,passed;
    char* ip,path;
    char temp[100];
    char portTemp[100];
    char pathTemp[100];
    // retains the information for the content length 
    char *ret;
    //Temp = argv[2]
    strcpy(temp, argv[2]);

    if (argc < 3 || argc > 4) {
        printf("**ERROR: invalid arguments \n");
        exit(EXIT_FAILURE);
    }

    if(strchr(temp,':')!= NULL){
        for(int i = 0;i<strlen(temp);i++){
            if(temp[i] == ':'){
              passed = i;
            }
            if(temp[i] == '/'){
                strncpy(portTemp, temp+passed+1,i-passed-1);
                strncpy(pathTemp, temp+i,strlen(temp)+1-i);
                break;
            }
        }
        strtok(temp, "/");
        strtok(temp, ":");
        ip = temp;
        port = atoi(portTemp);
   }else{
       for(int i = 0;i<strlen(temp);i++){
            if(temp[i] == '/'){
                strncpy(pathTemp, temp+i,strlen(temp)+1-i);
                break;
            }
        }
        strtok(temp, "/");
        ip = temp;
        port=80;
   }   
   
    // printf("\nIP: %s \n",ip);
    // printf("PORT NUM: %d \n",port);
    // printf("PATH: %s \n",pathTemp);

    // printf("\n");

    // Checks if Socket failed to create
    if(my_socket<0){
        printf("**ERROR: Socket Failed to Create. \n");
        exit(EXIT_FAILURE);
    }
    // else{
    //     printf("Socket created! \n");
    // }
    //Checks if IP is a valid IPv4
    if(inet_pton(AF_INET, ip, &addr)!=1){
        printf("**ERROR:Invalid Ipv4 address \n");
        exit(EXIT_FAILURE);
    }
    // else{
    //     printf("Ipv4 Valid \n");
    // }
    //Checks for Port
    if(!(port>=1 && port<=1023)){
        printf("**ERROR: Invalid port number \n");
        exit(EXIT_FAILURE);

    }
    // else{
    //     printf("Valid Port Number!\n");
    // }
    //Checks if the port number is valid
    
    //Specifying server's information
    server_address.sin_addr.s_addr = inet_addr(ip); // IP ADDRESS
    server_address.sin_family = AF_INET; // Type of IP version
    server_address.sin_port = htons(port); // Port

    // Socket Establish connection with the server
    is_connected = connect(my_socket,(struct sockaddr*)&server_address,sizeof(server_address));

    if (is_connected == -1) {
		printf("**ERROR: Socket failed to connect. \n");
        exit(EXIT_FAILURE);
	}
    // else{
    //     printf("Connection Esatblished!\n");
    // }

    // Creates HTTP Request and stored on 'request_template'
    if(argc == 4){
        if(strcmp(argv[3], "-h")== 0){
            sprintf(request_template, "HEAD %s HTTP/1.1\r\nHost: %s\r\n\r\n",pathTemp,argv[1]);
            option_h = 1;
        }else{
            printf("**ERROR: Invalid option. \n");
    		exit(EXIT_FAILURE);
        }
    }else{
        sprintf(request_template, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n",pathTemp,argv[1]);
    }

    // Socket sends the request to the server
    if( send(my_socket,request_template,strlen(request_template),0) < 0){
		printf("**ERROR: Socket failed to request. \n");
		exit(EXIT_FAILURE);
	}
    // else{
    //     printf("Send Work! \n");
    // }


    // Open output.dat file for writing
    fptr = fopen("output.dat", "w");
    if(!fptr){
        printf("**ERROR: File creation failed. \n");
		exit(EXIT_FAILURE);
    }



    n = recv(my_socket,data_buffer,BUFF_SIZE,0);
    header = strstr(data_buffer, "\r\n\r\n"); // Saves the ocurrance of the end of the header
    ret = strstr(data_buffer, "Content-Length:"); // Looks for the content lenght
    // Before getting the file content we must ensure the content is encoded in 'chunked'
    if(strstr(data_buffer, "Transfer-Encoding: chunked") != NULL){
        printf("**ERROR: Chunked Encoding not supported. \n");
		exit(EXIT_FAILURE);
    }
    if(header!= NULL){
         // Write contents of data buffer after "\r\n\r\n" sequence to output.dat file
         if(option_h != 1){
             // Writes the content after the end of the header
            fwrite(header + 4, 1, n - (header - data_buffer + 4), fptr);
            // Now it checks if the Header has 'Content-Length'
            if(ret != NULL){
                sscanf(ret, "Content-Length: %d", &content_length); 
            }
            // Saves the amount of bytes recived
            bytes_received = n - (header - data_buffer + 4);
         }else{
            printf("\n");
            printf(data_buffer);
            printf("\n");
         }   
    }

    if(option_h !=1){
        while(1){
            if(n==0 || content_length==bytes_received ){
             break;
            }
            if(n<0){
                printf("**ERROR: Received error");
                exit(EXIT_FAILURE);
            }
            n = recv(my_socket,data_buffer,BUFF_SIZE,0);
            fwrite(data_buffer, 1, n , fptr);
            bytes_received+=n;
        }
    }
  
    fclose(fptr);
    close(my_socket);
   
    return 0;
}

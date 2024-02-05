#include "proxy.h"
string http501 = "HTTP/1.1 501 Not Implemented\nThe requested method is '";
string http502 = "HTTP/1.1 502 Bad Gateway\r\n";
string http503 = "HTTP/1.1 503 Service Unavailable\r\n";
string http504 = "HTTP/1.1 504 Gateway Timeout\r\n";
string http400 = "HTTP/1.1 400 Bad Request\r\n";

bool updateFile = false;
bool endProcess = false;
void signalHandler(int signum){
    cout << "Interrupt signal Handler " <<endl;
    updateFile = true;
    //exit(signum); 
}
void proccessHandler(int signum){
    cout << "Process Interrupt signal Handler " <<endl;
    endProcess = true;
    //exit(signum); 
}
string getClientIP(int socket){
    struct sockaddr_in client_addr;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    string clientipstr = client_ip;
    //cout<< clientipstr <<endl;
    return clientipstr;
}
void updateLog(string logFile,string httpRequest,string ip,int httpReply,int contentLen){
    ofstream outfile;
    //cout <<logFile<<endl;
    outfile.open(logFile,ios::app);
    if (!outfile.is_open()) {
        cout << "Unable to open file." << endl;
        return;
    }  
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
    //2019-02-27T20:19:44.852Z 127.0.0.1 "GET http://ucsc.edu/ HTTP/1.1" 200 2326
    outfile <<timestap<<" "<< ip << " '"<< httpRequest << "' "<<httpReply<<" " <<contentLen << endl;
    outfile.close();

}
void readForbidden(string forbidden,map<string,int>& forbiddenSites){
    // Read from the text file
    ifstream readFile(forbidden);
    string line;
    int    count = 0;
    while (getline (readFile, line)) {
        count++;
        forbiddenSites.insert({line,count});
    }
    readFile.close();
}
int getResponseStatus(string s){
    int begin = s.find(" ")+1;
    int last = s.find_last_of(" ")-begin;
    string temp = s.substr(begin,last);
    return atoi(temp.c_str());
}
string getRequestHead(string s){
    int len = s.find("\r\n");
    string temp = s.substr(0,len);
    return temp;
}
bool checkRequest(string client_http_Request){
    string temp = client_http_Request.substr(0,client_http_Request.find('\n'));
    if(temp.find("GET")!=string::npos || temp.find("HEAD")!=string::npos){
        return true;
    }else{
        return false;
    }
}
string getHost(string& client_http_Request){
    string s;
    size_t end_pos = client_http_Request.find('\n', client_http_Request.find("Host"));
    size_t name_pos = client_http_Request.find(" ", client_http_Request.find("Host"));
    s = client_http_Request.substr(name_pos+1,end_pos-name_pos-2);
    //cout << "HOST: "<< s <<endl;
    string hostTemp;
    for(int i = 0;i<s.length();i++){
        if(s[i]!='\n' && s[i]!='\r'){
            hostTemp+=s[i];
        }
    }
    string host;
    for(int i = 0;i<hostTemp.length();i++){
        if(hostTemp[i]!=':'){
            host+=hostTemp[i];
        }else{
            break;
        }
    }
    //cout << host <<endl;
    return host;
}
int getPort(string& client_http_Request,string hostname){
    // cout << client_http_Request;
    string method = client_http_Request.substr(0,client_http_Request.find("\r"));
    int begin = method.find(hostname);
    if(method[begin+hostname.length()] == ':'){
        string temp;
        for(int i = begin+hostname.length()+1;i<method.length();i++){
            if(isdigit(method[i])){
                temp+=method[i];
            }else{
                break;
            }
        }
        return atoi(temp.c_str());
    }else{
        return 443;
    }
}
string getHostIP(string hostname){
    // cout << hostname <<endl;
    struct addrinfo hints,*result, *p;
    int status;
    char ip_str[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM; 
    status = getaddrinfo(hostname.c_str(), NULL, &hints, &result);
    if(status!=0){
        cerr << "getaddrinfo error: " << gai_strerror(status) <<endl;
        return "502 Bad Gateway";
    }
     for(p = result; p != NULL; p = p->ai_next) {
        void *addr;
        string ipver;
        // get pointer to the address itself
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        // convert IP address to a human-readable string
        inet_ntop(p->ai_family, addr, ip_str, sizeof(ip_str));
        // return the first valid IP address found
        if (strcmp(ip_str, "") != 0) {
            freeaddrinfo(result);
            return ip_str;
        }
     }
    freeaddrinfo(result);
    return "502 Bad Gateway";
}
void bridge(string ip,int port,string request,int client_socket, int& contentLength,string& httpresponse){
    int           len;
    int           content_length = 0;
    long          bytes_received = 0;
    int           ssl_socket;
    int           clientSend;
    char          buffer[BUFF_SIZE];
    bool          containsLength = true;
    string        port_str = to_string(port);

    ssl_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ssl_socket == -1) {
        cerr << "Error creating socket" <<endl;
        // exit(EXIT_FAILURE);
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip.c_str());

    // Connect to the server
    if (connect(ssl_socket, reinterpret_cast<const sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        cerr << "Error connecting to server" <<endl;
        clientSend = send(client_socket, http503.c_str(), http503.length(), 0);
        contentLength = http503.size();
        httpresponse = http503;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        return;
    }
    // Initialize Librerias
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX* ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if (ssl_ctx == nullptr) {
        cerr << "Error creating SSL context: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
        clientSend = send(client_socket, http502.c_str(), http502.length(), 0);
        contentLength = http502.size();
        httpresponse = http502;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        SSL_CTX_free(ssl_ctx);
        close(ssl_socket);
        return;
    }
    //Creates an SSL object using SSL_new()
    SSL* ssl = SSL_new(ssl_ctx);
    if (ssl == nullptr) {
        cerr << "Error creating SSL object: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
        clientSend = send(client_socket, http502.c_str(), http502.length(), 0);
        contentLength = http502.size();
        httpresponse = http502;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        close(ssl_socket);
        return;
    }
    // Sets Socket
    if (SSL_set_fd(ssl, ssl_socket) != 1) {
        cerr << "Error setting SSL file descriptor: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
        clientSend = send(client_socket, http502.c_str(), http502.length(), 0);
        contentLength = http502.size();
        httpresponse = http502;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        close(ssl_socket);
        return;
    }
    // SSL/TLS handshake happening here
    if (SSL_connect(ssl) != 1) {
        cerr << "Error establishing SSL connection: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
        clientSend = send(client_socket, http502.c_str(), http502.length(), 0);
        contentLength = http502.size();
        httpresponse = http502;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        close(ssl_socket);
        return;
    }
    // Sent request
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if(setsockopt(ssl_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        cerr<< "**ERROR: setsockopt failed" <<endl;
        clientSend = send(client_socket, http502.c_str(), http502.length(), 0);
        contentLength = http502.size();
        httpresponse = http502;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        close(ssl_socket);
        return;
        //exit(EXIT_FAILURE);
    }
    len = SSL_write(ssl, request.c_str(), request.size());
    if(len<=0){
        cerr << "Error sending request: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
        clientSend = send(client_socket, http400.c_str(), http400.length(), 0);
        contentLength = http400.size();
        httpresponse = http400;
        if(clientSend == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        close(ssl_socket);
        return;
    }
    // Received response
    long headerSize;
    int PacketCount = 0;
    //cout << "STARTING TO RECEIVED SSL REQUEST!"<<endl;
    while (true) {
        len = SSL_read(ssl, buffer, BUFF_SIZE);
        //cout << len <<endl;
        if(len <= 0){
            int ssl_error = SSL_get_error(ssl, len);
            if (errno == EWOULDBLOCK){
                if(PacketCount == 0){
                    clientSend = send(client_socket, http504.c_str(), http504.length(), 0);
                    contentLength = http504.size();
                    httpresponse = http504;
                    if(clientSend == -1) {
                        cerr << "Error sending data: " << strerror(errno) << endl;
                        break;
                    }
                    break;
                }

                break;
            }
            if(len == 0){
                break;
            }
        }
        buffer[len] = '\0';
        PacketCount++;
        bytes_received += len;

        //cout << "BUFFER: " << endl;
        //cout << buffer;
        clientSend = send(client_socket, buffer, len, 0);
        if(clientSend == -1) {
            if(errno == ECONNRESET || errno == EPIPE || errno == EINTR){
                cerr<< "CLIENT DOWN CLEAN UP"<<endl;
                break;
            }
        }
        if (content_length == 0) {
            //cout << "hello" <<endl;
            const char *header = strstr(buffer, "Content-Length: ");
            char *temp = strstr(buffer, "HTTP/");
            if (header != NULL) {
                //cout << "hello sscanf" <<endl;
                sscanf(header + strlen("Content-Length: "), "%d", &content_length);
                contentLength = content_length;
            }else{
                containsLength =false;
                //content_length = 1;  // Maybe required? to avoid overwriting the httpResponse
                char* header_end = strstr(buffer, "\r\n\r\n");
                headerSize = header_end-buffer+4;
            }
            if(temp!=NULL){
                char* ptr = strstr(temp, "\r\n");
                if(ptr){
                    int prefixLen = ptr - temp;
                    char prefix[prefixLen + 1];
                    strncpy(prefix, temp, prefixLen);
                    prefix[prefixLen] = '\0';
                    httpresponse = prefix;
                    content_length = 1; 
                }
            }
        }
        // if(containsLength){ // =======> MAYBER NOT NECCESSARY?
        //     if (bytes_received >= content_length) {
        //         //cout << "Break on: bytes_received >= content_length" <<endl;
        //         break;
        //     }
        // }
    }
    //cout << "HTTP RESPONSE: "<< httpresponse <<endl;
    if(containsLength == false){
        contentLength = abs(bytes_received);
    }
    if(request.find("HEAD")!=string::npos){
        content_length = bytes_received;
    }
    //cout << "CLEAN UP SSL" << endl;
    // Clean up

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ssl_ctx);
    close(ssl_socket);
}
bool checkForChars(string s){
  for (unsigned int i = 0; i < s.length(); i++) {
    if(!(isdigit(s[i]))){
      return true;
    }
  }
  return false;
}
void createDirectory(string path){
    // cout << "Create Directory " <<endl; 
    // cout << path <<endl;
    int    position = 0;
    int count = 0;
    string temp = "/" + path;
    string token;
    ofstream myfile; 

    while ((position = path.find('/')) != string::npos){
        token = path.substr(0, position);
        // cout<< token << endl;
        if (mkdir(token.c_str(), 0777) == -1 && errno != EEXIST) {
            perror("Error creating directory: " );
            exit(EXIT_FAILURE);
        }
        chdir(token.c_str());
        path.erase(0, position + 1);
        count ++;
    }
    // cout << temp.substr(temp.find_last_of('/')+1)  <<endl;
    myfile.open(temp.substr(temp.find_last_of('/')+1)); 
    myfile.close();
    while(count!=0){
        chdir("..");
        count--;
    }

}
void handleClientConnection(int clientSocket,string logFile,map<string,int> forbiddenSites){
    string client_http_Request;
    string responseTemp;
    string host;
    string ip;
    int port;
    int valread;
    //map<string,int> forbiddenSites;
    //readForbidden(forbidden,forbiddenSites);
    char buffer [BUFF_SIZE] = {0};
    valread = recv(clientSocket, buffer, BUFF_SIZE, 0);
    if (valread == -1) {
        cerr << "Error receiving data from client" << endl;
        return;
    }
    buffer[valread] = 0;
    client_http_Request = buffer;
    //cout << client_http_Request;
    // CHECK IF THE REQUEST HAS ANY HTTP METHOD THAT IS NOT HEAD OR GET
    if(checkRequest(client_http_Request)==false){
        string temp = client_http_Request.substr(0,client_http_Request.find('\n'));
        responseTemp = http501 + temp.substr(0,temp.find(" ")) + "' not implemented on this server.";
        send(clientSocket, responseTemp.c_str(), strlen(responseTemp.c_str()), 0); 
        close(clientSocket); // Should I close?
        return;
    }
    host = getHost(client_http_Request);
    //cout << "HOST: "<<host <<endl;
    port = getPort(client_http_Request,host);
    // CHECK IF THE HOST IS PART OF THE FORBIDDEN SITES
    if(forbiddenSites.find(host) != forbiddenSites.end()){
        responseTemp = "HTTP/1.1 403 Forbidden\r\n";
        send(clientSocket, responseTemp.c_str(), strlen(responseTemp.c_str()), 0);
        updateLog(logFile,getRequestHead(client_http_Request),getClientIP(clientSocket),403,responseTemp.size());
        close(clientSocket); 
        return; 
    }
    // CHECK IF THE HOST IS AN IPv4
    struct in_addr addr;
    if(inet_pton(AF_INET, host.c_str(), &addr)==1){
        ip = host;
    }else{
        ip = getHostIP(host);
    } 
    if(ip == "502 Bad Gateway"){
        responseTemp = "HTTP/1.1 502 Bad Gateway\r\n";
        send(clientSocket, responseTemp.c_str(), strlen(responseTemp.c_str()), 0);
        updateLog(logFile,getRequestHead(client_http_Request),getClientIP(clientSocket),502,responseTemp.size());
        close(clientSocket);
        return; 
    }
    //cout << "IP: "<< ip <<endl;
    //cout<< "SEND HTTPS REQUEST AND SEND BACK TO CLIENT" <<endl;
    string response;
    int contentLength;
    
    getPort(client_http_Request,host);
    bridge(ip,port,client_http_Request,clientSocket,contentLength,response);

    // cout << "Response from main func and content Length" <<endl;
    // cout << response << endl;
    // cout << "Content Length: " <<contentLength <<endl;
    updateLog(logFile,getRequestHead(client_http_Request),getClientIP(clientSocket),getResponseStatus(response),contentLength);

    //cout << "CLOSE SOCKET" <<endl;
    close(clientSocket);

}
void startConnection(int port,string forbidden,string logFile){
    struct sockaddr_in server_address;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        cerr << "**Error: creating server socket" << endl;
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        cerr << "Error binding to port" << endl;
        exit(EXIT_FAILURE);
    }
    //SOMAXCONN
    if (listen(server_socket, 100) == -1) {
        cerr << "Error listening for connections" << endl;
        exit(EXIT_FAILURE);
    }
    vector<thread> threads;
    map<string,int> forbiddenSites;
    readForbidden(forbidden,forbiddenSites);
    signal(SIGINT,signalHandler);
    // signal(SIGTERM,proccessHandler);
    // signal(SIGKILL,proccessHandler);

    // int flags = fcntl(server_socket, F_GETFL, 0);
    // fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    struct timeval timeout;
    timeout.tv_sec = 1800;
    timeout.tv_usec = 0;
    if(setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        cerr<< "**ERROR: setsockopt failed" <<endl;
    }

    cout << "Server is listening on port "<< port << endl;
    while (true) {
        begin:
        if(updateFile == false){
            struct sockaddr_in client_address;
            socklen_t client_address_size = sizeof(client_address);
            //cout << "Hang HERE?" <<endl;
            int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_size);
            if (client_socket == -1) {
                if (errno == EWOULDBLOCK || errno == EINTR) {
                    // No incoming connections yet, continue loop
                    //cout << "NO CONNETION YET "<<endl;
                    goto begin;
                }else{
                    cerr << "Error accepting client connection" << std::endl;
                    exit(EXIT_FAILURE);
                }
                //cout << "SIGNAL" <<endl;
            }else{
                //cout << "Accepted new client connection" << endl;
                threads.emplace_back(handleClientConnection, client_socket,logFile,forbiddenSites);
            }

        }else{
            cout << "Updating Forbidden File" <<endl;
            forbiddenSites.clear();
            readForbidden(forbidden,forbiddenSites);
            updateFile = false;
        }
    }
    // cout << "EXIT LOOP " <<endl;
    // for (auto& thread : threads) {
    //     thread.join();
    // }
    // close(server_socket);
    // cout << "GOODBYE" <<endl;
}
void Proxy::getArguments(int argc, char** argv){
    if(argc>4 || argc<4){
        cerr<< "**ERROR: Please enter the right amount of arguments 3." << endl;
        cerr<< "**EXAMPLE: ./myproxy listen_port forbidden_sites_file_path access_log_file_path" << endl;
        exit(EXIT_FAILURE);
    }
    if(checkForChars(argv[1])==true){
        cerr<< "**ERROR: Please enter the right format for the port, invalid port number '"<< argv[1]<< "'."<<endl;
        exit(EXIT_FAILURE);
    }
    ifstream file_1(argv[2]);
    if(!file_1.good()){
        cerr<< "ERROR: File '" << argv[2] << "' does not exist."<<endl;
        exit(EXIT_FAILURE);
    }
    ifstream file_2(argv[3]);
    if(!file_2.good()){
        string s = argv[3];
        createDirectory(s);
    }
    //Assigning values
    port = atoi(argv[1]);
    forbiddenSites_file = argv[2];
    log_file = argv[3];
}
void Proxy::startProxy(){
    startConnection(port,forbiddenSites_file,log_file);
}
void Proxy::test(){
    // Register signal handler for SIGINT
    // Infinite loop
    map<string,int> test;
    readForbidden(forbiddenSites_file,test);
    cout << "TEST BEFORE"<<endl;
    for (auto const& pair : test) {
        cout << "Key: " << pair.first << ", Value: " << pair.second << endl;
    }
    signal(SIGINT,signalHandler);
    // signal(SIGTERM,proccessHandler);
    // signal(SIGKILL,proccessHandler);
    while(1) {
        //cout << "Waiting for SIGINT...\n";
        if(updateFile == true){
            cout << "Updating Forbidden File" <<endl;
            readForbidden(forbiddenSites_file,test);
            for (auto const& pair : test) {
                cout << "Key: " << pair.first << ", Value: " << pair.second << endl;
            }
            updateFile = false;
        }
        if (endProcess == true){
            cout << "ENDING PROCESS" <<endl;
            break;
        }
        sleep(1);
    }
    cout << "GOODBYE" <<endl;

}

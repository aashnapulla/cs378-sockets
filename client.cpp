#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

int main(){
    int sockfd;
    struct sockaddr_in dest_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    dest_addr.sin_family = AF_INET; 
    dest_addr.sin_port = htons(3490); 
    dest_addr.sin_addr.s_addr = INADDR_ANY; 

    memset(&(dest_addr.sin_zero), '\0', 8);
    connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

    const char* msg = "Hello World!";
    send(sockfd, msg, strlen(msg), 0);
    cout << "Sent message" << endl;
    close(sockfd);
}
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
#define MYPORT 3490 // the port users will be connecting to

using namespace std;

int main(void)
{
    int sockfd, new_fd; 
    struct sockaddr_in my_addr; 
    struct sockaddr_in their_addr;
    int sin_size;


    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT); 
    my_addr.sin_addr.s_addr = INADDR_ANY; 
    memset(&(my_addr.sin_zero), '\0', 8); 

    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    listen(sockfd, 10);


    sin_size = sizeof(struct sockaddr_in);
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t*)&sin_size);
    char buffer[1024] = {0};
    read(new_fd, buffer, 1024);
    cout << "Received message: " << buffer << endl;
    return 0;
}
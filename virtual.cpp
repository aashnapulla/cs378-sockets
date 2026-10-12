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

struct ConnectMsg {
    uint32_t ipAddress;
    uint16_t udpPort;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    if(argc!=3) cerr << ">:( \n";

    int MYPORT = stoi(argv[1]);

    int udp_sockfd, tcp_sockfd; 
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;

    //UDP bind
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT); 
    my_addr.sin_addr.s_addr = INADDR_ANY; 
    memset(&(my_addr.sin_zero), '\0', 8); 

    if (bind(udp_sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))==-1) {
        cerr << "Binding error!";
    }

    //TCP bind
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET; 
    dest_addr.sin_port = htons(5000); 
    dest_addr.sin_addr.s_addr = inet_addr(argv[2]); 
    memset(&(dest_addr.sin_zero), '\0', 8);

    connect(tcp_sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));


    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    getsockname(tcp_sockfd, (struct sockaddr*)&local_addr, &addr_len);
    ConnectMsg msg;
    msg.ipAddress = local_addr.sin_addr.s_addr; 
    msg.udpPort = htons(MYPORT); 

    send(tcp_sockfd, &msg, sizeof(msg), 0);
    cout << "Sent connection message to ON with IP: " << inet_ntoa(local_addr.sin_addr) << " and UDP Port: " << MYPORT << endl;

    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(tcp_sockfd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            cerr << "Connection closed by ON or error occurred.\n";
            break;
        }
        cout << buffer << endl;
    }

    close(tcp_sockfd);
    close(udp_sockfd);
    return 0;
}
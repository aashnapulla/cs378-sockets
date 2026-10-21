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

struct LinkTuple {
    char id;
    uint32_t ipAddress;
    uint16_t udpPort;
    uint32_t cost;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    if(argc!=3) {
        cerr << ">:( \n";
        return 1;
    }

    int MYPORT = stoi(argv[1]);

    int udp_sockfd, tcp_sockfd; 
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;

    //TCP bind
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET; 
    dest_addr.sin_port = htons(5000); 
    dest_addr.sin_addr.s_addr = inet_addr(argv[2]); 
    memset(&(dest_addr.sin_zero), '\0', 8);

    if (connect(tcp_sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))==-1) {
        cerr << "Connection error!\n";
        return 1;
    }

    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    getsockname(tcp_sockfd, (struct sockaddr*)&local_addr, &addr_len);

    //UDP bind
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = local_addr.sin_addr.s_addr;
    memset(&(my_addr.sin_zero), '\0', 8);

    if (bind(udp_sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))==-1) {
        cerr << "Binding error!\n";
        return 1;
    }

    ConnectMsg msg;
    msg.ipAddress = local_addr.sin_addr.s_addr; 
    msg.udpPort = htons(MYPORT); 

    send(tcp_sockfd, &msg, sizeof(msg), 0);
    cout << "Sent connection message to ON with IP: " << inet_ntoa(local_addr.sin_addr) << " and UDP Port: " << MYPORT << endl;

    uint8_t numLinks;
    LinkTuple links[26];
    while (true) {
        if (recv(tcp_sockfd, &numLinks, 1, MSG_WAITALL) <= 0 || recv(tcp_sockfd, links, numLinks * sizeof(LinkTuple), MSG_WAITALL) <= 0) {
            cerr << "Connection closed by ON or error occurred.\n";
            break;
        }
        cout << "Received LINK-STATE: ";
        for (int i = 0; i < numLinks; i++) {
            cout << (i ? "," : "") << links[i].id << "=" << ntohl(links[i].cost);
        }
        cout << endl;
    }

    close(tcp_sockfd);
    close(udp_sockfd);
    return 0;
}
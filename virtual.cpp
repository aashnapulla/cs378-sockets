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
#include <sys/select.h>
#include <time.h>
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

struct LinkCost {
    char id;
    uint32_t cost;
} __attribute__((packed));

struct LSP {
    char origin;
    uint32_t seq;
    uint8_t count;
    LinkCost links[26];
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

    char myId = 0;
    uint32_t mySeq = 0;
    uint32_t lastSeq[26] = {0};
    int graph[26][26];
    memset(graph, -1, sizeof(graph));
    time_t lastSent = time(NULL), lastPrinted = time(NULL);

    uint8_t numLinks = 0;
    LinkTuple links[26];
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(tcp_sockfd, &readfds);
        FD_SET(udp_sockfd, &readfds);
        struct timeval tv = {1, 0};
        select(max(tcp_sockfd, udp_sockfd) + 1, &readfds, NULL, NULL, &tv);

        if (FD_ISSET(tcp_sockfd, &readfds)) {
            if (recv(tcp_sockfd, &numLinks, 1, MSG_WAITALL) <= 0 || recv(tcp_sockfd, links, numLinks * sizeof(LinkTuple), MSG_WAITALL) <= 0) {
                cerr << "Connection closed by ON or error occurred.\n";
                break;
            }
            cout << "Received LINK-STATE: ";
            for (int i = 0; i < numLinks; i++) {
                if (links[i].cost == 0) myId = links[i].id;
                cout << (i ? "," : "") << links[i].id << "=" << ntohl(links[i].cost);
            }
            cout << endl;
            lastSent = time(NULL) - 8;
        }

        if (FD_ISSET(udp_sockfd, &readfds)) {
            LSP lsp;
            recv(udp_sockfd, &lsp, sizeof(lsp), 0);
            int o = lsp.origin - 'A';
            if (ntohl(lsp.seq) <= lastSeq[o]) {
                cout << "LSP from " << lsp.origin << " seq " << ntohl(lsp.seq) << ": old, dropped" << endl;
            } else {
                cout << "LSP from " << lsp.origin << " seq " << ntohl(lsp.seq) << ": new, flooding" << endl;
                lastSeq[o] = ntohl(lsp.seq);
                for (int j = 0; j < 26; j++) graph[o][j] = -1;
                graph[o][o] = 0;
                for (int j = 0; j < lsp.count; j++) graph[o][lsp.links[j].id - 'A'] = ntohl(lsp.links[j].cost);
                for (int i = 0; i < numLinks; i++) {
                    if (links[i].cost == 0) continue;
                    struct sockaddr_in nb_addr;
                    nb_addr.sin_family = AF_INET;
                    nb_addr.sin_port = links[i].udpPort;
                    nb_addr.sin_addr.s_addr = links[i].ipAddress;
                    memset(&(nb_addr.sin_zero), '\0', 8);
                    sendto(udp_sockfd, &lsp, sizeof(lsp), 0, (struct sockaddr *)&nb_addr, sizeof(struct sockaddr));
                }
            }
        }

        if (myId && time(NULL) - lastSent >= 10) {
            LSP lsp;
            lsp.origin = myId;
            lsp.seq = htonl(++mySeq);
            lsp.count = 0;
            for (int i = 0; i < numLinks; i++) {
                if (links[i].cost == 0) continue;
                lsp.links[lsp.count].id = links[i].id;
                lsp.links[lsp.count].cost = links[i].cost;
                lsp.count++;
            }
            sendto(udp_sockfd, &lsp, sizeof(lsp), 0, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
            cout << "Sent own LSP seq " << mySeq << endl;
            lastSent = time(NULL);
        }

        if (time(NULL) - lastPrinted >= 10) {
            int n = 0;
            for (int i = 0; i < 26; i++) if (lastSeq[i]) n = i + 1;
            cout << "Known graph:" << endl;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) cout << (j ? "\t" : "") << graph[i][j];
                cout << endl;
            }
            lastPrinted = time(NULL);
        }
    }

    close(tcp_sockfd);
    close(udp_sockfd);
    return 0;
}
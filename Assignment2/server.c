#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>
#include <time.h>


#define PORT 9000

int main() {
    int serverFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverFD < 0) {
        fprintf(stderr, "Error creating socket\n");
        exit(1);
    }

    if (setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error setting socket options for reusing address\n");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "Error binding socket\n");
        exit(1);
    } else {
        printf("Time server started:\n");
    }

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    while (1) {
        char buffer[1024];
        recvfrom(serverFD, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);

        // Convert IP address from binary to text form
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

        printf("Time request from: %s:%d\n", clientIP, ntohs(clientAddr.sin_port));

        time_t rawTime = time(NULL);
        char* rawTimeStr = ctime(&rawTime);

        sendto(serverFD, rawTimeStr, strlen(rawTimeStr), 0, (struct sockaddr *)&clientAddr, clientAddrLen);
    }

    // Close connection
    close(serverFD);

    return 0;
}
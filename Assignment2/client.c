#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>

#define SERVER_PORT 9000

int main() {
    int clientFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientFD < 0) {
        fprintf(stderr, "Error creating socket\n");
        exit(1);
    }

    // Reuse address
    if (setsockopt(clientFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error setting socket options for reusing address\n");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char requestString[] = "Time request";

    sendto(clientFD, requestString, sizeof(requestString), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    char responseString[1024];
    recvfrom(clientFD, responseString, sizeof(responseString), 0, 0, 0);

    printf("Server time: %s\n", responseString);

    // Close connection
    close(clientFD);

    return 0;
}
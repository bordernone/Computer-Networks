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
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    while (1) {
        char buffer[1024];
        printf("Enter time request: ");
        fgets(buffer, sizeof(buffer), stdin);

        sendto(clientFD, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

        recvfrom(clientFD, buffer, sizeof(buffer), 0, 0, 0);
        printf("Time: %s\n", buffer);
    }

    // Close connection
    close(clientFD);

    return 0;
}
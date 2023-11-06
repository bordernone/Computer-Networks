#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>
#include <time.h>
#include <signal.h>

#define PORT 6000

int serverFD;

void handleClient(int clientFD, struct sockaddr_in clientAddr) {
    printf("[%s:%d] Connected\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    char buffer[256];
    while (1) {
        // Receive a message from the client.
        bzero(buffer, sizeof(buffer));
        int bytes = recv(clientFD, buffer, sizeof(buffer), 0);
        if (bytes == 0 || strncmp(buffer, "BYE!", 4) == 0) {
            // Client has closed the connection.
            printf("[%s:%d] Disconnected\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            close(clientFD);
            break;
        }
        printf("[%s:%d] Received message: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);

        // Send back the same message to the client.
        send(clientFD, buffer, strlen(buffer), 0);
        printf("[%s:%d] Sent message: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);
    }
}

int main() {
    // Create a TCP socket for the server.
    serverFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFD < 0) {
        fprintf(stderr, "Error creating socket\n");
        exit(1);
    }

    // Enable reusing the same address and port for multiple runs.
    if (setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error setting socket options for reusing address\n");
        exit(1);
    }

    // Define the server's address and port.
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified address and port.
    if (bind(serverFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "Error binding socket\n");
        exit(1);
    } else {
        printf("Server has started\n");
    }

    // Set up the socket to listen for incoming connections.
    if (listen(serverFD, 5) < 0) {
        fprintf(stderr, "Error listening on socket\n");
        exit(1);
    } else {
        printf("Server is listening on port %d\n", PORT);
    }

    while (1) {
        // Accept a new client connection.
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientFD = accept(serverFD, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientFD < 0) {
            fprintf(stderr, "Error accepting client connection\n");
            exit(1);
        }

        // Handle the client.
        int pid = fork();
        if (pid == 0) {
            // Child process.
            handleClient(clientFD, clientAddr);
            break;
        } else {
            // Parent process.
            close(clientFD);
        }
    }
    return 0;
}
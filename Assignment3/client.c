#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(-1);
    }

    // Set socket options to reuse the same address and port.
    int value = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
        perror("setsockopt");
        close(fd);
        exit(-1);
    }

    // Server address.
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(6000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server.
    if (connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        close(fd);
        exit(-1);
    }

    // Get my IP address and port.
    struct sockaddr_in myAddr;
    bzero(&myAddr, sizeof(myAddr));
    socklen_t len = sizeof(myAddr);
    if (getsockname(fd, (struct sockaddr *)&myAddr, &len) < 0) {
        perror("getsockname");
        close(fd);
        exit(-1);
    }
    char myIP[16];
    inet_ntop(AF_INET, &myAddr.sin_addr, myIP, sizeof(myIP));
    unsigned int myPort = ntohs(myAddr.sin_port);

    printf("Local IP address: %s\n", myIP);
    printf("Local port: %u\n", myPort);

    // Send and receive data with error handling.
    char buffer[256];
    while (1) {
        printf("Enter a message: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove trailing newline char from buffer, fgets does not remove it.
        ssize_t bytes_sent = send(fd, buffer, strlen(buffer), 0);
        if (bytes_sent < 0) {
            perror("send");
            close(fd);
            exit(-1);
        }

        if (strncmp(buffer, "BYE!", 4) == 0) {
            // Client has closed the connection.
            printf("Connection closed from client side\n");
            close(fd);
            break;
        }

        // Receive a message from the server.
        bzero(buffer, sizeof(buffer));
        ssize_t bytes_received = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("recv");
            close(fd);
            exit(-1);
        }
        printf("Server response: %s\n", buffer);
    }

    close(fd);
    return 0;
}

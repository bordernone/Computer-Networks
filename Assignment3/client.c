#include <stdio.h> // header for input and output from console : printf, perror
#include<string.h> // strcmp
#include<sys/socket.h> // for socket related functions
#include<arpa/inet.h> // htons
#include <netinet/in.h> // structures for addresses

#include<unistd.h> // contains fork() and unix standard functions
#include<stdlib.h>


#include<unistd.h> // header for unix specic functions declarations : fork(), getpid(), getppid()
#include<stdlib.h> // header for general functions declarations: exit()

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(-1);
    }

    // Set socket options to reuse the same address and port.
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
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
        exit(-1);
    }

    // Get my ip address and port
    struct sockaddr_in myAddr;
    bzero(&myAddr, sizeof(myAddr));
    socklen_t len = sizeof(myAddr);
    getsockname(fd, (struct sockaddr *) &myAddr, &len);
    char myIP[16];
    inet_ntop(AF_INET, &myAddr.sin_addr, myIP, sizeof(myIP));
    unsigned int myPort = ntohs(myAddr.sin_port);

    printf("Local ip address: %s\n", myIP);
    printf("Local port : %u\n", myPort);

    // Send a message to the server.
    char buffer[256];
    while (1) {
        printf("Enter a message: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove trailing newline char from buffer, fgets does not remove it.
        send(fd, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "BYE!", 4) == 0) {
            // Client has closed the connection.
            printf("Connection closed from client side\n");
            close(fd);
            break;
        }

        // Receive a message from the server.
        bzero(buffer, sizeof(buffer));
        recv(fd, buffer, sizeof(buffer), 0);
        printf("Server response: %s\n", buffer);
    }

    return 0;
}
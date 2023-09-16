#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>
#include <time.h>
#include <signal.h>

#define PORT 9000

int serverFD;

void exitHandler(int signum) {
    // Close the socket and release resources.
    close(serverFD);
    printf("Time server stopped\n");
    exit(0);
}

int main() {
    // Create a UDP socket for the server.
    serverFD = socket(AF_INET, SOCK_DGRAM, 0);
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
        printf("Time server started:\n");
    }

    // Setup signal handler to close the socket when Ctrl+C is pressed.
    if (signal(SIGINT, exitHandler) == SIG_ERR) {
        perror("Error setting up signal handler to hanlde Ctrl+C\n");
        exit(1);
    }

    // Set up variables for handling client requests.
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Enter a loop to continuously handle incoming requests.
    while (1) {
        char buffer[1024];

        // Receive the request from a client.
        int msgLength = recvfrom(serverFD, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (msgLength < 0) {
            fprintf(stderr, "Error receiving message\n");
            exit(1);
        }

        // Convert the client's IP address from binary to text form.
        char clientIP[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN) == NULL) {
            fprintf(stderr, "Error converting IP address to string\n");
            exit(1);
        }

        // Print information about the incoming request.
        printf("Time request from: %s:%d\n", clientIP, ntohs(clientAddr.sin_port));

        // Get the current time and format it as a string.
        time_t rawTime = time(NULL);
        char* rawTimeStr = ctime(&rawTime);

        // Send the current time back to the client.
        if (sendto(serverFD, rawTimeStr, strlen(rawTimeStr), 0, (struct sockaddr *)&clientAddr, clientAddrLen) < 0) {
            fprintf(stderr, "Error sending message\n");
            exit(1);
        }
    }

    return 0;
}

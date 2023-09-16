#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>

#define SERVER_PORT 9000

int main() {
    // Create a UDP socket for the client
    int clientFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientFD < 0) {
        fprintf(stderr, "Error creating socket\n");
        exit(1);
    }

    // Enable reusing the same address and port for multiple runs
    if (setsockopt(clientFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error setting socket options for reusing address\n");
        exit(1);
    }

    // Define the server's address and port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Prepare the request message. Any message will do
    char requestString[] = "Time request";

    // Send the request message to the server, blocking call
    sendto(clientFD, requestString, sizeof(requestString), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    // Receive the server's response and save it in a buffer. Also a blocking call
    char responseString[1024];
    recvfrom(clientFD, responseString, sizeof(responseString), 0, 0, 0);

    // Display the server's response (assumes it contains the time)
    printf("Server time: %s\n", responseString);

    // Close the socket and release resources
    close(clientFD);

    return 0;
}

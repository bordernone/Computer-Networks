#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define DEBUG 0
#define MAX_METHOD_LENGTH 10
#define MAX_VERSION_LENGTH 10
#define MAX_URI_LENGTH 1024
#define MAX_HEADER_LENGTH 4096
#define MAX_BODY_LENGTH 4096
#define MAX_REQUEST_LENGTH (MAX_METHOD_LENGTH + MAX_VERSION_LENGTH + MAX_URI_LENGTH + MAX_HEADER_LENGTH + MAX_BODY_LENGTH)
#define CRLFCRLF "\r\n\r\n"

// Struct to store the parsed HTTP request
struct HttpRequest {
    char method[MAX_METHOD_LENGTH];
    char uri[MAX_URI_LENGTH];
    char version[MAX_VERSION_LENGTH];
    char headers[MAX_HEADER_LENGTH]; // Not used for now
    char body[MAX_BODY_LENGTH]; // Not used for now
};

// Struct to store the tuple of a char array and its length
struct CharLengthTuple {
    char *msg;
    int length;
};

// Struct to store the client socket file descriptor and its address, easy to pass to a thread
struct ClientTuple {
    int clientFD;
    struct sockaddr_in clientAddr;
};

// The directory to serve static files from. This is relative to the directory where the executable is run
char STATIC_DIR[] = "assignment3_files";

// Get the size of a file
int fileSize(FILE *file) {
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    return size;
}

// Get the file path of a URI, combine the STATIC_DIR and the URI
char *getFilePath(char *uri, bool freeOld) { // freeOld is a helper to free the old uri
    char *filePath = calloc(strlen(STATIC_DIR) + strlen(uri) + 1, sizeof(char));
    strcat(filePath, STATIC_DIR); // add the STATIC_DIR
    strcat(filePath, uri); // add the URI
    filePath[strlen(STATIC_DIR) + strlen(uri)] = '\0';
    if (freeOld) {
        free(uri);
    }
    return filePath;
}

// Parse the URI, if it ends with /, append index.html
char *parseURI(char *uri, bool freeOld) {
    // if ends with /, append index.html
    if (uri[strlen(uri) - 1] == '/') {
        char *newURI = calloc(strlen(uri) + strlen("index.html") + 1, sizeof(char));
        strcpy(newURI, uri);
        strcat(newURI, "index.html");
        if (freeOld) {
            free(uri);
        }
        return newURI;
    } else {
        return uri;
    }
}

// Get the content type of a resource
char* getContentType(char *resource) {
    char *extension = strrchr(resource, '.');
    if (extension == NULL) {
        // No extension, return text/plain. This can be useful if server wants to respond with a string, probably not useful for this assignment
        return "text/plain";
    }
    if (strcmp(extension, ".html") == 0) {
        return "text/html";
    } else if (strcmp(extension, ".png") == 0) {
        return "image/png";
    } else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
        return "image/jpg";
    } else if (strcmp(extension, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(extension, ".css") == 0) {
        return "text/css";
    } else if (strcmp(extension, ".js") == 0) {
        return "application/javascript";
    } else {
        return "text/plain";
    }
}

// Check if the content should be sent as binary
bool isBinary(const char *contentType) {
    return strcmp(contentType, "image/png") == 0 || strcmp(contentType, "image/jpg") == 0 || strcmp(contentType, "image/gif") == 0 || strcmp(contentType, "image/jpeg") == 0;
}

// Strip off the body of a request since we don't need it for this assignment
char* stripOffBody(char *request) {
    // Find the end of the header
    char *headerEndsAt = strstr(request, CRLFCRLF);
    if (headerEndsAt == 0) {
        printf("Error parsing request\n");
        exit(1);
    }
    // Calculate the length of the header
    int lengthBeforeCRLFCRLF = headerEndsAt - request;
    // Copy the header into a new string
    char *requestWithoutBody = calloc(lengthBeforeCRLFCRLF + 1, sizeof(char));
    strncpy(requestWithoutBody, request, lengthBeforeCRLFCRLF);
    requestWithoutBody[lengthBeforeCRLFCRLF] = '\0'; // Ensure the string is null-terminated
    return requestWithoutBody;
}

// Get the current date and time in proper format
struct CharLengthTuple getCurrentFormattedDate() {
    time_t currentTime;
    time(&currentTime);

    char formattedDate[30];
    strftime(formattedDate, sizeof(formattedDate), "%a %b %d %H:%M:%S %Y", localtime(&currentTime));

    size_t resultSize = strlen(formattedDate) + 1;
    char *result = calloc(resultSize, sizeof(char));

    if (result == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    snprintf(result, resultSize, "%s", formattedDate);

    struct CharLengthTuple resultTuple;
    resultTuple.msg = result;
    resultTuple.length = resultSize - 1;

    return resultTuple;
}

// Build the response message based on the content type and body. Runs only when the file exists
struct CharLengthTuple buildResponseMessage(const char* contentType, const char* body, int bodyLength) {
    // Define the format of the response header
    const char* responseFormat = "HTTP/1.0 200 OK\r\nServer: SimpleHTTPServer\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n";

    struct CharLengthTuple dateTuple = getCurrentFormattedDate();

    int headerLength = snprintf(NULL, 0, responseFormat, dateTuple.msg, contentType, bodyLength);
    int totalLength = headerLength + bodyLength; // Add 1 for the null terminator

    // Allocate memory for the response message
    char *responseMessage = calloc(totalLength, sizeof(char));

    if (responseMessage == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }

    // Build the response header
    sprintf(responseMessage, responseFormat, dateTuple.msg, contentType, bodyLength);

    // Copy the binary body directly into the response message
    memcpy(responseMessage + headerLength, body, bodyLength);

    // Free memory
    free(dateTuple.msg);
    return (struct CharLengthTuple) {responseMessage, totalLength};
}

// Remove the end of line characters from a string
void removeEndOfLine(char *str) {
    size_t len = strlen(str);

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
        }
    }
}

// Parse the HTTP request
struct HttpRequest parseHttpRequest(char *request) {
    struct HttpRequest httpRequest;
    char *saveptr;
    // Remove any body content
    char *requestWithoutBody = stripOffBody(request);
    char *token = strtok_r(requestWithoutBody, " ", &saveptr);
    strncpy(httpRequest.method, token, MAX_METHOD_LENGTH);
    token = strtok_r(NULL, " ", &saveptr);
    strncpy(httpRequest.uri, parseURI(token, false), MAX_URI_LENGTH);
    token = strtok_r(NULL, " ", &saveptr);
    // Remove any end of line characters
    char *version = calloc(strlen(token) - 2, sizeof(char));
    strncpy(version, token, strlen(token) - 2);
    removeEndOfLine(version);
    strncpy(httpRequest.version, version, MAX_VERSION_LENGTH);

    return httpRequest;
}

// Get the contents of a file along with its length
struct CharLengthTuple getFileContents(const char *filePath, const char* contentType) {
    FILE *file;
    // May need to open the file in binary mode depending on the content type
    if (isBinary(contentType)) {
        if (DEBUG) {
            printf("Opening file in binary mode %s\n", filePath);
        }
        file = fopen(filePath, "rb");
    } else {
        if (DEBUG) {
            printf("Opening file in text mode %s\n", filePath);
        }
        file = fopen(filePath, "r");
    }
    if (file == NULL) {
        if (DEBUG) {
            printf("File not found: %s\n", filePath);
        }
        return (struct CharLengthTuple) {"", -1};
    }
    int size = fileSize(file);
    char *body = calloc(size + 1, sizeof(char));
    fread(body, size, 1, file);
    fclose(file);
    body[size] = '\0';
    return (struct CharLengthTuple) {body, size};
}

// Check if a file exists
bool fileExists(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

// Get the 404 message
struct CharLengthTuple get404Message() {
    // Define the format of the response header
    char *format = "HTTP/1.0 404 Not Found\r\nServer: SimpleHTTPServer\r\nDate: %s\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\n404 Not Found";
    struct CharLengthTuple dateTuple = getCurrentFormattedDate(); // Get the current date
    int messageLength = snprintf(NULL, 0, format, dateTuple.msg);
    char *message = calloc(messageLength + 1, sizeof(char));
    sprintf(message, format, dateTuple.msg); // Build the message
    message[messageLength] = '\0';
    free(dateTuple.msg);
    return (struct CharLengthTuple) {message, messageLength};
}

// Handle a request independently
void handleRequest(struct ClientTuple clientTuple) {
    int clientFD = clientTuple.clientFD;

    // Read the request
    char *request = calloc(MAX_REQUEST_LENGTH, sizeof(char));
    int bytesRead = read(clientFD, request, MAX_REQUEST_LENGTH);
    if (bytesRead < 0) {
        printf("Error reading\n");
        return;
    }
    
    // Parse the request
    struct HttpRequest httpRequest = parseHttpRequest(request);

    // Print the request
    if (DEBUG){
        printf("%s\n", request);
        printf("=====================================\n");    
        printf("Method: %s\n", httpRequest.method);
        printf("URI: %s\n", httpRequest.uri);
        printf("Version: %s\n", httpRequest.version);
    }

    if (strcmp(httpRequest.method, "GET") != 0) {
        printf("Error: Only GET method is supported\n");
        return;
    }

    // Prepare the response
    char *contentType = getContentType(httpRequest.uri);
    char *filePath = getFilePath(httpRequest.uri, false);
    struct CharLengthTuple fileContents = getFileContents(filePath, contentType);

    struct CharLengthTuple responseHelper;

    // Check if file exists
    if (fileContents.length == -1) {
        // Build the response
        responseHelper = get404Message();
    } else {
        // Build the response
        responseHelper = buildResponseMessage(contentType, fileContents.msg, fileContents.length);
    }

    // Send the response
    int bytesSent = send(clientFD, responseHelper.msg, responseHelper.length, 0);
    if (bytesSent < 0) {
        printf("Error sending\n");
        return;
    } else if (DEBUG) {
        printf("Sent %d bytes\n", bytesSent);
    }

    // Print server log. Format: 127.0.0.1 [13/Mar/2022 12:23:06] "GET /index.html HTTP/1.1" 200
    time_t t;
    time(&t);
    char formattedTime[30];
    strftime(formattedTime, sizeof(formattedTime), "%d/%b/%Y %H:%M:%S", localtime(&t));
    printf("%s [%s] \"%s %s %s\" %d\n", inet_ntoa(clientTuple.clientAddr.sin_addr), formattedTime, httpRequest.method, httpRequest.uri, httpRequest.version, fileContents.length == -1 ? 404 : 200);

    // Free memory
    free(filePath);
    free(responseHelper.msg);
    if (fileContents.length != -1)
        free(fileContents.msg);

    // Close the connection
    close(clientFD);
}

// Handle a request in a thread
void *handleRequestThread(void *arg) {
    struct ClientTuple clientTuple = *(struct ClientTuple *)arg;
    handleRequest(clientTuple);
    return NULL; // Return NULL such that the thread runs in detached mode
}

int main() {
    // Create a socket, TCP
    int serverFD = socket(AF_INET, SOCK_STREAM, 0);

    // Reuse the socket
    int reuse = 1;
    if (setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("Error reusing socket\n");
        return 1;
    }

    // Bind the socket to an IP address and port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80); // Port 80
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error binding. There may be other services running on 80. Try closing other programs and run this as root.\n");
        return 1;
    }

    // Listen for connections
    if (listen(serverFD, 10) < 0) {
        printf("Error listening\n");
        return 1;
    }

    // Accept a connection
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientFD = accept(serverFD, (struct sockaddr *) &clientAddr, &clientAddrLen);
        if (clientFD < 0) {
            printf("Error accepting\n");
            return 1;
        } else {
            // Handle the request in a new thread
            pthread_t thread;
            struct ClientTuple *clientTuple = malloc(sizeof(struct ClientTuple));
            clientTuple->clientFD = clientFD;
            clientTuple->clientAddr = clientAddr;
            pthread_create(&thread, NULL, handleRequestThread, clientTuple);
        }
    }
    
    // Close the socket
    close(serverFD);
    return 0;
}

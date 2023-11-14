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

struct HttpRequest {
    char method[MAX_METHOD_LENGTH];
    char uri[MAX_URI_LENGTH];
    char version[MAX_VERSION_LENGTH];
    char headers[MAX_HEADER_LENGTH];
    char body[MAX_BODY_LENGTH];
};

struct CharLengthTuple {
    char *msg;
    int length;
};

char STATIC_DIR[] = "assignment3_files";

int fileSize(FILE *file) {
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    return size;
}

char *getFilePath(char *uri, bool freeOld) {
    char *filePath = malloc(strlen(STATIC_DIR) + strlen(uri) + 1);
    strcat(filePath, STATIC_DIR);
    strcat(filePath, uri);
    filePath[strlen(STATIC_DIR) + strlen(uri)] = '\0';
    if (freeOld) {
        free(uri);
    }
    return filePath;
}

char *parseURI(char *uri, bool freeOld) {
    // if ends with /, append index.html
    if (uri[strlen(uri) - 1] == '/') {
        char *newURI = malloc(strlen(uri) + strlen("index.html") + 1);
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

char* getContentType(char *resource) {
    char *extension = strrchr(resource, '.');
    if (extension == NULL) {
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

bool isBinary(const char *contentType) {
    return strcmp(contentType, "image/png") == 0 || strcmp(contentType, "image/jpg") == 0 || strcmp(contentType, "image/gif") == 0 || strcmp(contentType, "image/jpeg") == 0;
}

char* stripOffBody(char *request) {
    char *headerEndsAt = strstr(request, CRLFCRLF);
    if (headerEndsAt == 0) {
        printf("Error parsing request\n");
        exit(1);
    }
    int lengthBeforeCRLFCRLF = headerEndsAt - request;
    
    char *requestWithoutBody = malloc(lengthBeforeCRLFCRLF + 1);
    strncpy(requestWithoutBody, request, lengthBeforeCRLFCRLF);
    requestWithoutBody[lengthBeforeCRLFCRLF] = '\0';
    return requestWithoutBody;
}

struct CharLengthTuple getCurrentFormattedDate() {
    time_t currentTime;
    time(&currentTime);

    char formattedDate[30];
    strftime(formattedDate, sizeof(formattedDate), "%a %b %d %H:%M:%S %Y", localtime(&currentTime));

    size_t resultSize = strlen(formattedDate) + 1;
    char *result = (char *)malloc(resultSize);

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

struct CharLengthTuple buildResponseMessage(const char* contentType, const char* body, int bodyLength) {
    const char* responseFormat = "HTTP/1.0 200 OK\r\nServer: SimpleHTTPServer\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n";

    struct CharLengthTuple dateTuple = getCurrentFormattedDate();

    int headerLength = snprintf(NULL, 0, responseFormat, dateTuple.msg, contentType, bodyLength);
    int totalLength = headerLength + bodyLength + 1; // Add 1 for the null terminator

    char *responseMessage = (char *)malloc(totalLength);

    if (responseMessage == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }

    // Build the response header
    sprintf(responseMessage, responseFormat, dateTuple.msg, contentType, bodyLength);

    // Copy the binary body directly into the response message
    memcpy(responseMessage + headerLength, body, bodyLength);

    // Ensure the response message is null-terminated
    responseMessage[totalLength - 1] = '\0';

    // Free memory
    free(dateTuple.msg);
    return (struct CharLengthTuple) {responseMessage, totalLength};
}


struct HttpRequest parseHttpRequest(char *request) {
    struct HttpRequest httpRequest;
    char *saveptr;

    char *requestWithoutBody = stripOffBody(request);
    char *token = strtok_r(requestWithoutBody, " ", &saveptr);
    strncpy(httpRequest.method, token, MAX_METHOD_LENGTH);
    token = strtok_r(NULL, " ", &saveptr);
    strncpy(httpRequest.uri, parseURI(token, false), MAX_URI_LENGTH);
    token = strtok_r(NULL, " ", &saveptr);
    strncpy(httpRequest.version, token, MAX_VERSION_LENGTH);

    return httpRequest;
}


struct CharLengthTuple getFileContents(const char *filePath, const char* contentType) {
    FILE *file;
    if (isBinary(contentType)) {
        file = fopen(filePath, "rb");
    } else {
        file = fopen(filePath, "r");
    }
    if (file == NULL) {
        if (DEBUG) {
            printf("File not found: %s\n", filePath);
        }
        return (struct CharLengthTuple) {"", -1};
    }
    int size = fileSize(file);
    char *body = malloc(size + 1);
    fread(body, size, 1, file);
    body[size] = '\0';
    fclose(file);
    return (struct CharLengthTuple) {body, size};
}

bool fileExists(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

struct CharLengthTuple get404Message() {
    char *format = "HTTP/1.0 404 Not Found\r\nServer: SimpleHTTPServer\r\nDate: %s\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\n404 Not Found";
    struct CharLengthTuple dateTuple = getCurrentFormattedDate();
    int messageLength = snprintf(NULL, 0, format, dateTuple.msg);
    char *message = malloc(messageLength + 1);
    sprintf(message, format, dateTuple.msg);
    message[messageLength] = '\0';
    free(dateTuple.msg);
    return (struct CharLengthTuple) {message, messageLength};
}

void handleRequest(int clientFD) {
    // Read the request
    char *request = malloc(MAX_REQUEST_LENGTH);
    int bytesRead = read(clientFD, request, MAX_REQUEST_LENGTH);
    if (bytesRead < 0) {
        printf("Error reading\n");
        return;
    }

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
    printf("Content-Type: %s\n", contentType);
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

    // Free memory
    free(filePath);
    free(responseHelper.msg);
    if (fileContents.length != -1)
        free(fileContents.msg);

    // Close the connection
    close(clientFD);
}

void *handleRequestThread(void *arg) {
    int clientFD = *(int *)arg;
    handleRequest(clientFD);
    return NULL;
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
    serverAddr.sin_port = htons(80);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error binding\n");
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
            pthread_t thread;
            pthread_create(&thread, NULL, handleRequestThread, &clientFD);
        }
    }
    
    // Close the socket
    close(serverFD);
    return  0;
}

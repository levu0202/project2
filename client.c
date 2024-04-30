#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAX_PATH_LENGTH 1024

void error(const char * msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char * argv[]) {
    int sockfd, portNum, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    uint32_t fileSize, responseCode;

    if (argc < 3) {
       fprintf(stderr, "Usage: %s hostname port filename\n", argv[0]);
       exit(1);
    }

    portNum = atoi(argv[2]); 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portNum);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    char file_path[MAX_PATH_LENGTH];

    while(1) {
        if (read(sockfd, &responseCode, sizeof(responseCode)) < 0)
            error("ERROR reading response code from socket");
        responseCode = ntohl(responseCode);
        printf("Response code: %d\n", responseCode);

        if (responseCode == 0) {
            uint32_t urlLength;
            if (read(sockfd, &urlLength, sizeof(urlLength)) < 0)
                error("ERROR reading URL length from socket");
            urlLength = ntohl(urlLength);

            char *message = malloc(urlLength + 1); 
            if (message == NULL)
                error("Memory allocation failed");

            if (read(sockfd, message, urlLength) < 0) {
                free(message);
                error("ERROR reading URL from socket");
            }
            printf("URL length : %d\n", urlLength);
            message[urlLength] = '\0'; 
            printf("Received message: %s\n", message);
            free(message);
        } else {
            char *errorMessage = malloc(1024 + 1);
            n = read(sockfd, errorMessage, sizeof(errorMessage)-1);
            if (n < 0) 
                error("ERROR reading error message from server");
            // errorMessage[n] = '\0'; 
            printf("Error from server: %s\n", errorMessage);
            free(errorMessage);
        }
        fgets(file_path, sizeof(file_path), stdin);
        size_t len = strlen(file_path);
        if (file_path[len - 1] == '\n') {
            file_path[len - 1] = '\0';  
        }
        FILE *file = fopen(file_path, "rb");
        if (file == NULL) error("Error opening file");
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        rewind(file);

        char *buffer = malloc(fileSize + 1);
        if (buffer == NULL) error("Memory allocation failed");
        
        size_t bytesRead = fread(buffer, 1, fileSize, file);
        if (bytesRead != fileSize)
            error("Error reading file");

        uint32_t netFileSize = htonl(fileSize);
        if (write(sockfd, &netFileSize, sizeof(netFileSize)) < 0) 
            error("ERROR writing file size to socket");

        if (write(sockfd, buffer, fileSize) < 0) 
            error("ERROR writing file content to socket");
        free(buffer);  
        fclose(file);

    }

    close(sockfd);
    return 0;
}

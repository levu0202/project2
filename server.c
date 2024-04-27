#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void error(const char *msg) {
    perror(msg);
    exit(1);
}



int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portNum;
    socklen_t clilen;
    char *buffer;
    struct sockaddr_in server_addr, client_addr;
    unsigned int fileSize;


    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    portNum = atoi(argv[1]);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portNum);

    if (bind(sockfd, (struct sockaddr *) & server_addr, sizeof(server_addr)) < 0) error("Error on binding");

    listen(sockfd, 5);
    clilen = sizeof(client_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen);
    if (newsockfd < 0) 
        error("ERROR on accept");

    
    int n = read(newsockfd, &fileSize, sizeof(fileSize));
    if (n < 0) {
        free(buffer);
        error("ERROR reading from socket");

    } 
    fileSize = ntohl(fileSize); 

    buffer = malloc(fileSize + 1);
    if (buffer == NULL) error("Memory allocation failed");
    n = recv(newsockfd, buffer, fileSize, MSG_WAITALL);
    if (n < fileSize) {
        free(buffer);
        error("ERROR reading full data from socket");
    }
    // printf("Binary data received:\n");
    // for (unsigned int i = 0; i < fileSize; i++) {
    //     print_binary(buffer[i]);
    //     printf(" ");  // Add space between bytes
    //     if ((i + 1) % 8 == 0) printf("\n");  // New line every 8 bytes
    // }

    FILE *file = fopen("received_qr_code.png", "wb");
    if (file == NULL) {
        free(buffer);
        error("Failed to open file for writing");
    }
    fwrite(buffer, 1, fileSize, file);
    fclose(file);
    free(buffer);

    // Execute ZXing command
    char command[512];
    sprintf(command, "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner received_qr_code.png");
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        error("Failed to run command");
    }
    char resultBuffer[1024];
    int lineCount = 0;
    while (fgets(resultBuffer, sizeof(resultBuffer), fp) != NULL) {
        lineCount++;
        if (lineCount == 3) {
            uint32_t responseCode = htonl(0); // Assume success response code for this example
            uint32_t urlLength = htonl(strlen(resultBuffer));
            printf("%s", resultBuffer);
            write(newsockfd, &responseCode, sizeof(responseCode));
            write(newsockfd, &urlLength, sizeof(urlLength));
            write(newsockfd, resultBuffer, strlen(resultBuffer));
            break; // Stop after processing the third line
        }
    }
    pclose(fp);

    close(newsockfd);
    close(sockfd);
    return 0; 


}
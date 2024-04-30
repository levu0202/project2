#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define MAX_FILE_SIZE 4294967295

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void *handle_client(int newsockfd) {
    for (;;) {
    unsigned int fileSize;
    int n = read(newsockfd, &fileSize, sizeof(fileSize));
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            uint32_t responseCode = htonl(2);  
            const char *timeoutMessage = "Timeout: No activity within the allowed period.\n";
            uint32_t msgLength = htonl(strlen(timeoutMessage));
            write(newsockfd, &responseCode, sizeof(responseCode));
            write(newsockfd, &msgLength, sizeof(msgLength));
            write(newsockfd, timeoutMessage, strlen(timeoutMessage));
        } else {
            // Other errors
            perror("ERROR reading from socket");
        }
        close(newsockfd);
        return NULL;
    }
    fileSize = ntohl(fileSize);
    if (fileSize == 0 || fileSize > MAX_FILE_SIZE) {
        uint32_t responseCode = htonl(1);
        char *errorMessage = "File size exceeds limit.\n";
        write(newsockfd, &responseCode, sizeof(responseCode));
        uint32_t msgLength = htonl(strlen(errorMessage));
        write(newsockfd, &msgLength, sizeof(msgLength));
        write(newsockfd, errorMessage, strlen(errorMessage));
        close(newsockfd);
        continue;
    }

    char *buffer = malloc(fileSize + 1);
    if (buffer == NULL) {
        close(newsockfd);
        error("Memory allocation failed");
        return NULL;
    }

    n = recv(newsockfd, buffer, fileSize, MSG_WAITALL);
    if (n < fileSize) {
        free(buffer);
        close(newsockfd);
        error("ERROR reading full data from socket");
        return NULL;
    }
    FILE *file = fopen("received_qr_code.png", "wb");
    if (file == NULL) {
        free(buffer);
        close(newsockfd);
        error("Failed to open file for writing");
        return NULL;
    }
    fwrite(buffer, 1, fileSize, file);
    fclose(file);
    free(buffer);

    char command[512];
    sprintf(command, "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner received_qr_code.png");
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        close(newsockfd);
        error("Failed to run command");
        return NULL;
    }
    char resultBuffer[BUFFER_SIZE];
    int lineCount = 0;
    while (fgets(resultBuffer, sizeof(resultBuffer), fp) != NULL) {
        lineCount++;
        if (lineCount == 3) {
            uint32_t responseCode = htonl(0); 
            uint32_t urlLength = htonl(strlen(resultBuffer));
            write(newsockfd, &responseCode, sizeof(responseCode));
            write(newsockfd, &urlLength, sizeof(urlLength));
            write(newsockfd, resultBuffer, strlen(resultBuffer));
            break; 
        }
    }
    pclose(fp);
    }

    close(newsockfd);
    return NULL;

    }

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <port> <max_clients>\n", argv[0]);
        exit(1);
    }
    int newsockfd;
    pid_t childpid;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    int max_clients = atoi(argv[2]);
    int timeout_sec = atoi(argv[3]); 

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, '\0',
           sizeof(server_addr));
    socklen_t clilen = sizeof(client_addr);
    int portNum = atoi(argv[1]);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portNum);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        error("Error on binding");
    }

    listen(sockfd, max_clients);
    struct timeval timeout;
    timeout.tv_sec = timeout_sec; 
    timeout.tv_usec = 0;  
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        error("Failed to set socket receive timeout");
    }
    int count = 0;
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clilen);
        if (newsockfd < 0) {
            exit(1);
        }
        if (count >= max_clients) {
            uint32_t responseCode = htonl(1);  
            char *errorMessage = "Server full: Maximum clients reached.";
            printf("%s\n", errorMessage);
            uint32_t msgLength = htonl(strlen(errorMessage));
            write(newsockfd, &responseCode, sizeof(responseCode));
            write(newsockfd, &msgLength, sizeof(msgLength));
            write(newsockfd, errorMessage, strlen(errorMessage));
            close(newsockfd);
            continue;
        }
        count++;
        printf("Connection accepted from %s:%d\n",
        inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port));

        printf("Clients connected: %d\n\n",
            count);

        if ((childpid = fork()) == 0) {
            close(sockfd);
            handle_client(newsockfd);
        

        } 

    }
    close(newsockfd);
    return 0;
}

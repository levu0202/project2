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
    char buffer[1024];
    struct sockaddr_in server_addr, client_addr;
    int n;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
    }

    bzero((char *) &server_addr, sizeof(server_addr));
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

    bzero(buffer, 1024);
    n = read(newsockfd, buffer, 1023);
    if (n < 0) error("ERROR reading from socket");

    char command[512];
    sprintf(command, "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner %s", buffer);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        error("Failed to run command");
    }
    int count = 0;
    while (fgets(buffer, 1023, fp) != NULL) {
        count ++;
        if (count == 3) {
            n = write(newsockfd, buffer, strlen(buffer));
            if (n < 0) {
                perror("ERROR writing to socket");
                break;
        }
        }
    }

    pclose(fp);

    close(newsockfd);
    close(sockfd);
    return 0; 


}
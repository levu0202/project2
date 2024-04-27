#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


void error(const char * msg) {
    perror(msg);
    exit(0);
}


void print_binary(unsigned char byte) {
    for (int i = 7; i >= 0; i--) {
        printf("%c", (byte & (1 << i)) ? '1' : '0');
    }
}


int main(int argc, char * argv[]) {
    int sockfd, portNum, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    uint32_t fileSize;

    char *buffer;
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portNum = atoi(argv[2]); 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        exit(0);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portNum);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    FILE *file = fopen("qr_code.png", "rb");
    if (file == NULL) error("Error opening file");
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    rewind(file);

    buffer = malloc(fileSize + 1);
    if (buffer == NULL) error("Memory allocation failed");
    
    
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize)
        error("Error reading file");
    // printf("Binary representation of the file:\n");
    // for (size_t i = 0; i < fileSize; i++) {
    //     print_binary(buffer[i]);
    //     printf(" ");  // Space between bytes for readability
    //     if ((i + 1) % 8 == 0) printf("\n");  // Newline every 8 bytes for better readability
    // }
    // printf("\n");

    uint32_t netFileSize = htonl(fileSize);
    if (write(sockfd, &netFileSize, sizeof(netFileSize)) < 0) 
        error("ERROR writing file size to socket");

    if (write(sockfd, buffer, fileSize) < 0) 
         error("ERROR writing file content to socket");
    // buffer[fileSize] = '\0'; // Null-terminate the string
    // printf("Received filename: %s\n", buffer);
    free(buffer);  // Free the buffer after it's sent
    fclose(file);  // Close the file as soon as it's no longer needed

    // Read the response code and URL length
    uint32_t responseCode, urlLength;
    if (read(sockfd, &responseCode, sizeof(responseCode)) < 0)
        error("ERROR reading response code from socket");
    responseCode = ntohl(responseCode);

    if (read(sockfd, &urlLength, sizeof(urlLength)) < 0)
        error("ERROR reading URL length from socket");
    urlLength = ntohl(urlLength);

    char *message = malloc(urlLength + 1); // Allocate buffer for the received URL/message
    if (message == NULL)
        error("Memory allocation failed");

    if (read(sockfd, message, urlLength) < 0) {
        free(message);
        error("ERROR reading URL from socket");
    }
    message[urlLength] = '\0'; // Null-terminate the string
    printf("Response code: %d\n", responseCode);
    printf("url length: %d\n", urlLength);
    printf("Received message: %s\n", message);
    free(message);
    close(sockfd);
    return 0;
}
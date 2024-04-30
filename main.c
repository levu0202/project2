#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#define MAX_PATH_LENGTH 1024
int active_clients = 0;
int max_clients;

typedef struct {
    char *hostname;
    int port;
    // char *file_path;
} client_args;

void start_server(int port, int max_clients) {
    char cmd[256];
    sprintf(cmd, "./server %d %d", port, max_clients);
    if (system(cmd) < 0) {
        perror("Failed to start server");
        exit(1);
    }
}

void *client_session(void *arg) {
    client_args *args = (client_args *)arg;

    char cmd[MAX_PATH_LENGTH + 50];
    sprintf(cmd, "./client %s %d", args->hostname, args->port);
    if (system(cmd) < 0) {
        perror("Failed to start client");
    }
    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <max_clients>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    max_clients = atoi(argv[2]);
    int time_out_val = atoi(argv[3]);

    // pid_t pid = fork();
    // if (pid == 0) {
    //     start_server(port, max_clients);
    //     exit(0);
    // } else if (pid < 0) {
    //     perror("Fork failed");
    //     exit(1);
    // }
    // start_server(port, max_clients);
    // char file_path[MAX_PATH_LENGTH];
    // fgets(file_path, sizeof(file_path), stdin);
    // size_t len = strlen(file_path);
    // if (file_path[len - 1] == '\n') {
    //     file_path[len - 1] = '\0';  
    // }

    client_args *args = malloc(sizeof(client_args));  
    if (args == NULL) {
        perror("Failed to allocate memory for thread arguments");
    }
    args->hostname = strdup("localhost");
    args->port = port;
    // args->file_path = strdup(file_path);
    client_session(args);
 
    return 0;
}

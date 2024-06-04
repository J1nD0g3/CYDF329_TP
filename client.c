#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER 2048

int main(int argc, char *argv[]){
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER];

    if(argc != 3){
        fprintf(stderr, "Usage: %s <server_ip> <port>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    char* server_ip = argv[1];
    int port = atoi(argv[2]);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");

    while(1){
        memset(buffer, 0, MAX_BUFFER);
        fgets(buffer, MAX_BUFFER, stdin);

        if(strncmp(buffer, "EXIT", 4) == 0){
            break;
        }

        send(client_socket, buffer, strlen(buffer), 0);
        memset(buffer, 0, MAX_BUFFER);
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if(bytes_received <= 0){
            break;
        }
        printf("%s", buffer);
    }
    close(client_socket);
    return 0;
}

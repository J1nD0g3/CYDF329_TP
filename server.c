#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_KEYS 10
#define MAX_BUFFER 2048

typedef struct{
    char key[1024];
    char value[1024];
} KeyValue;

KeyValue store[MAX_KEYS];
int key_count = 0;
pthread_mutex_t lock;

void *handle_client(void *arg){
    int client_socket = *((int*)arg);
    char buffer[MAX_BUFFER];
    while(1){
        memset(buffer, 0, MAX_BUFFER);
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if(bytes_received <= 0){
            break;
        }

        if(strncmp(buffer, "set", 3) == 0){
            char key[1024], value[1024];
            sscanf(buffer + 4, "%s %s", key, value);

            pthread_mutex_lock(&lock);
            int update = 0;
            int i;
            for(i = 0; i < key_count; ++i){
                if(strcmp(store[i].key, key) == 0){
                    strcpy(store[i].value, value);
                    update = 1;
                    break;
                }
            }

            if(update == 1){
                pthread_mutex_unlock(&lock);
                send(client_socket, "+OK\r\n", 5, 0);
            }
            else if(key_count >= MAX_KEYS){
                pthread_mutex_unlock(&lock);
                send(client_socket, "NO MORE KEYS..\r\n", 16, 0);
            }
            else{
                strcpy(store[key_count].key, key);
                strcpy(store[key_count].value, value);
                key_count++;
                pthread_mutex_unlock(&lock);

                send(client_socket, "+OK\r\n", 5, 0);
            }
        }else if(strncmp(buffer, "get", 3) == 0){
            char key[1024];
            sscanf(buffer + 4, "%s", key);

            pthread_mutex_lock(&lock);
            int i;
            for(i=0; i < key_count; ++i){
                if(strcmp(store[i].key, key) == 0){
                    char response[1050];
                    sprintf(response, "$%lu\r\n%s\r\n", strlen(store[i].value), store[i].value);
                    send(client_socket, response, strlen(response), 0);
                    break;
                }
            }
            if(i == key_count){
                send(client_socket, "$-1\r\n", 5, 0);
            }
            pthread_mutex_unlock(&lock);
        }else{
            send(client_socket, "-ERR unknown command\r\n", 21, 0);
            shutdown(client_socket, SHUT_RDWR);
            break;
        }
    }
    close(client_socket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t thread_id;

    if(argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket, 5) < 0){
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_init(&lock, NULL) != 0){
        perror("Mutex initialization failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while(1){
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if(client_socket < 0){
            perror("Accept failed");
            continue;
        }

        pthread_create(&thread_id, NULL, handle_client, (void*)&client_socket);
        pthread_detach(thread_id);
    }

    pthread_mutex_destroy(&lock);
    close(server_socket);
    return 0;
}

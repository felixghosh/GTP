#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define SIZE 1024

#define PORT 8080

void handle_connection(int socket);
int getCommand(char *clientMessage);
void getFile(int conn_sock_fd);
void putFile(int conn_sock_fd);
void terminateConnection(int conn_sock_fd);
void send_file(char *fp, int conn_sock_fd);

int main(){
    int e;

    int listen_sock_fd, conn_sock_fd;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;

    listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock_fd < 0){
        perror("\033[1;31m[-]Error in socket");
        exit(1);
    }
    if(setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    printf("\033[0;32m[+]\033[0m Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = PORT;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    e = bind(listen_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(e < 0){
        perror("\033[1;31m[-]\033[0mError in bind");
        exit(1);
    }
    printf("\033[0;32m[+]\033[0m Binding successfull.\n");

    if(listen(listen_sock_fd, 10) == 0)
        printf("\033[0;32m[+]\033[0m Listening....\n");
    else{
        perror("\033[1;31m[-]\033[0mError in listening");
        exit(1);
    }

    addr_size = sizeof(new_addr);
    while(true){
        conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&new_addr, &addr_size);
        char addr[40];
        inet_ntop(AF_INET, &new_addr.sin_addr, addr, 40);
        printf("\033[0;32m[+]\033[0m Client connected! Address: %s\n", addr);
        handle_connection(conn_sock_fd);
    }
    return 0;
}

void handle_connection(int conn_sock_fd){
    bool finished = false;

    while(!finished){
        printf("\033[0;33m[/]\033[0m Awaiting command from client\n");
        char clientMessage[4];
        int x =recv(conn_sock_fd, clientMessage, 4, 0);
        if(x == 0){
            terminateConnection(conn_sock_fd);
            finished = true;
            break;
        }
        int command = getCommand(clientMessage);
        switch (command){
			case 0:
				getFile(conn_sock_fd);
				break;
			case 1:
				putFile(conn_sock_fd);
				break;
			case 2:
				terminateConnection(conn_sock_fd);
				finished = true;
				break;

			default:
				break;
        }
    }
    printf("\033[0;36m[o]\033[0m Closing connection to client\n");
    close(conn_sock_fd);
    return;
}

int getCommand(char *clientMessage){
    if(strcmp(clientMessage, "GET") == 0)
        return 0;
    else if (strcmp(clientMessage, "PUT") == 0)
        return 1;
    else if (strcmp(clientMessage, "EXT") == 0)
        return 2;
    return -1;
}

void getFile(int conn_sock_fd){
    printf("\033[0;32m[+]\033[0m Command received: \033[0;32mGET\033[0m\n");
    char fp[100];
    char okBuf[3] = "OK";
    send(conn_sock_fd, okBuf, sizeof okBuf, 0);
    printf("\033[0;33m[/]\033[0m Awaiting file path\n");
    recv(conn_sock_fd, fp, 100, 0);
    printf("\033[0;32m[+]\033[0m File path received: \033[0;33m%s\033[0m\n", fp);
    if (access(fp, F_OK) != -1){
        printf("\033[0;32m[+]\033[0m File exists on server. Sending file!\n");
        send_file(fp, conn_sock_fd);
        printf("\033[0;32m[+]\033[0m File sent!\n");
    }
}

void putFile(int conn_sock_fd){
    printf("\033[0;32m[+]\033[0m Command received: \033[0;32mPUT\033[0m\n");
    char fp[100];
    char okBuf[3] = "OK";
    send(conn_sock_fd, okBuf, sizeof okBuf, 0);
    printf("\033[0;33m[/]\033[0m Awaiting file path\n");
    recv(conn_sock_fd, fp, 100, 0);
    printf("\033[0;32m[+]\033[0m File path received: \033[0;33m%s\033[0m\n", fp);
    size_t file_size;
    printf("\033[0;33m[/]\033[0m Awating file size\n");
    recv(conn_sock_fd, &file_size, sizeof file_size, 0);
    printf("\033[0;32m[+]\033[0m File size received: \033[0;33m%lu\033[0m\n", file_size);
    char* data = calloc(1, file_size + 1);
    FILE* f = fopen(fp, "wb");
    printf("\033[0;32m[+]\033[0m Receiving file\n");
    size_t bytes_received = 0;
    char* temp = calloc(1, file_size + 1);
    while(bytes_received < file_size){
        memset(temp, 0, sizeof temp);
        size_t prev_index = bytes_received;
        bytes_received += recv(conn_sock_fd, temp, file_size, 0);
        for(int i = 0; i < bytes_received - prev_index; i++)
            data[prev_index + i] = temp[i];
    }
    printf("\033[0;32m[+]\033[0m File received!\n");
    fputs(data, f);
    fclose(f);
}

void terminateConnection(int conn_sock_fd){
    printf("\033[0;36m[o]\033[0m Command received: \033[0;32mEXT\033[0m\n");
}

void send_file(char *fp, int conn_sock_fd){
    struct stat file_stats;
    stat(fp, &file_stats);
    size_t file_size = file_stats.st_size;
    send(conn_sock_fd, &file_size, sizeof(size_t), 0);
    int file_fd = open(fp, O_RDONLY);
    size_t bytes_sent = 0;
    off_t *offset;
    printf("file size: %lu\n", file_size);
    while(bytes_sent < file_size)
        bytes_sent += sendfile(conn_sock_fd, file_fd, offset, file_size);
}
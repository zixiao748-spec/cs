#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, BUFFER_SIZE, 0);

    char method[16], path[1024];
    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") != 0) {
        char *msg = "HTTP/1.1 400 Bad Request\r\n\r\n";
        send(client_socket, msg, strlen(msg), 0);
        close(client_socket);
        exit(0);
    }

    char *file_path = path + 1;

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        char *msg = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_socket, msg, strlen(msg), 0);
        close(client_socket);
        exit(0);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char header[1024];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            file_size);

    send(client_socket, header, strlen(header), 0);

    char file_buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, file_buffer, bytes, 0);
    }

    fclose(file);
    close(client_socket);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) return 1;

    int port = atoi(argv[1]);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    while (1) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (fork() == 0) {
            close(server_socket);
            handle_client(client_socket);
        }
        close(client_socket);
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    close(server_socket);
    return 0;
}

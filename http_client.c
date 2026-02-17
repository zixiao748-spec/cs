#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

void write_output(const char *msg) {
    FILE *f = fopen("output", "wb");
    if (f) {
        fwrite(msg, 1, strlen(msg), f);
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write_output("INVALIDPROTOCOL");
        return 0;
    }

    char *url = argv[1];

    if (strncmp(url, "http://", 7) != 0) {
        write_output("INVALIDPROTOCOL");
        return 0;
    }

    char host[1024], path[2048];
    int port = 80;

    char *p = url + 7;
    char *slash = strchr(p, '/');

    if (!slash) {
        write_output("INVALIDPROTOCOL");
        return 0;
    }

    strncpy(host, p, slash - p);
    host[slash - p] = '\0';
    strcpy(path, slash);

    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port = atoi(colon + 1);
    }

    struct hostent *server = gethostbyname(host);
    if (!server) {
        write_output("NOCONNECTION");
        return 0;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        write_output("NOCONNECTION");
        return 0;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        write_output("NOCONNECTION");
        close(sockfd);
        return 0;
    }

    char request[4096];
    sprintf(request, "GET %s HTTP/1.0\r\n\r\n", path);
    send(sockfd, request, strlen(request), 0);

    FILE *out = fopen("output", "wb");
    if (!out) return 0;

    char buffer[BUFFER_SIZE];
    int bytes;
    int header_done = 0;

    while ((bytes = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        if (!header_done) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                header_done = 1;
                body += 4;

                if (strstr(buffer, "404 Not Found")) {
                    fclose(out);
                    write_output("FILENOTFOUND");
                    close(sockfd);
                    return 0;
                }

                fwrite(body, 1, bytes - (body - buffer), out);
            }
        } else {
            fwrite(buffer, 1, bytes, out);
        }
    }

    fclose(out);
    close(sockfd);
    return 0;
}

CC=gcc
CFLAGS=-Wall -std=c11

all: http_client http_server

http_client: http_client.c
	$(CC) $(CFLAGS) http_client.c -o http_client

http_server: http_server.c
	$(CC) $(CFLAGS) http_server.c -o http_server

clean:
	rm -f http_client http_server output

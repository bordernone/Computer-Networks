CC = gcc
CFLAGS = -Wall -Werror

all: client.out server.out

client.out: client.c
	$(CC) $(CFLAGS) -o client.out client.c

server.out: server.c
	$(CC) $(CFLAGS) -o server.out server.c

clean:
	rm -f client.out server.out
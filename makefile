CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGETS = server client

all: $(TARGETS)

server : server.c
	$(CC) $(CFLAGS) -o server server.c

client : client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f $(TARGETS)


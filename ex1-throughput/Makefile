CC=gcc
CFLAGS=-O4 -g

COMMON=common.h client.h server.h
all:clean client server

client: client.cpp
	$(CC) $(CFLAGS) $< $(COMMON) -o $@

server: server.cpp
	$(CC) $(CFLAGS) $< $(COMMON) -o $@

clean:
	rm -rf ./client ./server

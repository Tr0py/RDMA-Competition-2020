CC=gcc
CFLAGS=-O4 -g

all:client server

hello.elf:hello.c
	$(CC) $< $(CFLAGS) -o $@

client: client.cpp
	$(CC) $(CFLAGS) $< -o $@

server: server.cpp
	$(CC) $(CFLAGS) $< -o $@

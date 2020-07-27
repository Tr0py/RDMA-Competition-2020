#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <math.h>

#define MAXLINE 4096
#define BUFFERSIZE 0x100000

int InitializeBuffer(char** pbuffer)
{
    *pbuffer = (char*)mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((long)*pbuffer == -1) {
        perror("mmap");
        exit(1);
    }
    memset(*pbuffer, 0, BUFFERSIZE);
    return 0;
}


int sizeSend(int fd, char* buffer, int length, int flag, int size)
{
    int res, nleft = length;
    char* ptr = buffer;
    while (nleft > 0) {
        res = send(fd, ptr, size, flag);
        if (res < 0) {
            perror("send");
            exit(1);
        }
        nleft -= res;
        ptr += res;
        //fprintf(stderr, "[autoSend] Sent %d bytes, %d bytes left.\n", res, nleft);
    }
    return length;
}
int autoSend(int fd, char* buffer, int length, int flag)
{
    int res, nleft = length;
    char* ptr = buffer;
    while (nleft > 0) {
        res = send(fd, ptr, nleft, flag);
        if (res < 0) {
            perror("send");
            exit(1);
        }
        nleft -= res;
        ptr += res;
        //fprintf(stderr, "[autoSend] Sent %d bytes, %d bytes left.\n", res, nleft);
    }
    return length;
}

int autoRecv(int fd, char* buffer, int length, int flag)
{
    int res, nleft = length;
    char* ptr = buffer;
        //fprintf(stderr, "[autoRecv] Start receiving %d bytes.\n", length);
    while (nleft > 0) {
        //fprintf(stderr, "[autoRecv] receiving %d bytes.\n", length);
        res = recv(fd, ptr, nleft, flag);
        if (res < 0) {
            perror("recv");
            exit(1);
        }
        if (res == 0) {
            printf("Connection terminated.\n");
            exit(0);
        }
        nleft -= res;
        ptr += res;
        //fprintf(stderr, "[autoRecv] Received %d bytes, %d bytes left.\n", res, nleft);
    }
    return length;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define MAXLINE 4096

int main(int argc, char **argv)
{
    int listenfd, connfd; //listenfd as listen, accepted socket is connfd
    struct sockaddr_in servaddr; //0.0.0.0
    char* buff[4096];
    int n;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    if (listen(listenfd, 10) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    printf("======waiting for client's request======\n");
    if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1)
    {
        printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
        exit(1);
    }
        //int flag = 1;
        //int result = setsockopt(connfd,        /* socket affected */
        //                        IPPROTO_TCP,   /* set option at TCP level */
        //                        TCP_NODELAY,   /* name of option */
        //                        (char *)&flag, /* the cast is historical cruft */
        //                        sizeof(int));  /* length of option value */
        //if (result < 0)
        //{
        //    perror("setsockopt");
        //    return 1;
        //}
    while (1) {
        printf("begin recv\n");
        n = recv(connfd, buff, MAXLINE, 0);
        if (n <= 0) {
            fprintf(stderr, "socket broken.\n");
            perror("recv");
            return 0;
        }
        buff[n] = '\0';
        printf("received: %s\n", buff);
    }
    close(connfd);
    close(listenfd);
    return 0;
}


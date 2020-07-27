#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/timeb.h>
#include<sys/time.h>
#define MAXLINE 4096

static inline unsigned long long GetNTime(void)
{
    unsigned long long tsc;
    __asm__ __volatile__(
        "rdtscp;"
        "shl $32, %%rdx;"
        "or %%rdx, %%rax"
        : "=a"(tsc)
        :
        : "%rcx", "%rdx");

    return tsc;
}

int main(int argc, char **argv)
{
    char recvline[4096], sendline[4096];
    struct sockaddr_in servaddr;

    if (argc != 2)
    {
        printf("usage: ./client <ipaddress>\n");
        return 0;
    }

    char buf[] = "test", recvbuf[1024];
    struct timeval t, tend;
    unsigned long long secDiff;
        int sockfd, n;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
            return 0;
        }

        int flag = 1;
        int result = setsockopt(sockfd,        /* socket affected */
                                IPPROTO_TCP,   /* set option at TCP level */
                                TCP_NODELAY,   /* name of option */
                                (char *)&flag, /* the cast is historical cruft */
                                sizeof(int));  /* length of option value */
        if (result < 0)
        {
            perror("setsockopt");
            return 1;
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(6666);
        if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        {
            printf("inet_pton error for %s\n", argv[1]);
            return 0;
        }
        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
            return 0;
        }
        while (1) {
            gettimeofday(&t, NULL);
            scanf("%s", buf);
            
            buf[strlen(buf)] = '\0';
            if (send(sockfd, buf, strlen(buf), 0) < 0)
            {
                printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
                return 0;
            }
            //int res = recv(sockfd, recvbuf, 4, 0);
            gettimeofday(&tend, NULL);
            secDiff = (tend.tv_sec - t.tv_sec) * 1000000 + tend.tv_usec - t.tv_usec;
            printf("%lf\n", (double)secDiff/100000);
        }
        close(sockfd);

    return 0;
}



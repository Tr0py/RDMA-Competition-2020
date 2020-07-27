
#include "server.h"

int main(int argc, char **argv)
{
    int listenfd, connfd; //listenfd as listen, accepted socket is connfd
    struct sockaddr_in servaddr; //0.0.0.0
    char* buff[4096];
    char* buffer;
    int n;

    /* Initialize send/receive buffer. */
    InitializeBuffer(&buffer);
    /* Initialize socket and listen at 0.0.0.0:6666 */
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

    /* Help client know the RTT. */
    int halo = 5;
    printf("Helping client to measure RTT...\n");
    while (halo--) {
        n = autoRecv(connfd, buffer, 5, 0);
        autoSend(connfd, "done", 5, 0);
    }

    /* Every time we receives BUFFERSIZE bytes of data we reply an "done"
     * to let client know we got it.*/
    while (1) {
        printf("begin recv\n");
        n = autoRecv(connfd, buffer, BUFFERSIZE, 0);
        if (n < 0) {
            fprintf(stderr, "socket broken.\n");
            perror("recv");
            return 0;
        }
        else if (n == BUFFERSIZE) {
            autoSend(connfd, "done", 5, 0);
            printf("Received %lx Bytes.\n", BUFFERSIZE);
        }
        else {
            fprintf(stderr, "[panic] what is client doing?\n");
            exit(1);
        }

    }
    close(connfd);
    close(listenfd);
    return 0;
}


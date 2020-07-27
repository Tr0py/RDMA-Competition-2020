#include "client.h"

double rtt = 0;
double GetThroughput(int fd, int size, char* buffer);

int main(int argc, char **argv)
{
    char recvline[4096], sendline[4096];
    struct sockaddr_in servaddr;
    double throughput;

    if (argc != 2)
    {
        printf("usage: client <ipaddress>\n");
        return 0;
    }

    char buf[] = "test", recvbuf[1024];
    struct timeval t, tend;
    unsigned long long secDiff;
    int sockfd, n;
    char* buffer;
    char buff[4096];

    /* Initialize socket. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
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

    /* Initialize send/receive buffer. */
    InitializeBuffer(&buffer);

    /* Try to get the RTT */
    int halo = 5;
    int rttsum = 0;

    while (halo--) {
        gettimeofday(&t, NULL);
        autoSend(sockfd, "done", 5, 0);
        autoRecv(sockfd, buffer, 5, 0);
        gettimeofday(&tend, NULL);
        secDiff = (tend.tv_sec - t.tv_sec) * 1000000 + tend.tv_usec - t.tv_usec;
        rttsum += secDiff;
        //printf("[debug]\trtt = %dms\n", secDiff);
    }
    rtt = (double)rttsum / 5.0;
    //printf("[debug]\trtt avr = %lf ms\n", rtt);




    /* Warm-up round. */
    throughput = GetThroughput(sockfd, 2048, buffer);
    throughput = GetThroughput(sockfd, 2048, buffer);

    

    /* Testing round. */
    for (int i=0; i <= 10; i++) {
        int size = 1 << i;
        int j;
        double avr = 0, sum = 0, avrnew = 0;

        /* Firstly we run 2 rounds to get an average. */

        for (j=0; j < 2; j++) {
            throughput = GetThroughput(sockfd, size, buffer);
            sum += throughput;
        }

        avr = sum / (double)j;

        /* Here we do extra rounds until variation < 1%. */
        do {
            //printf("[debug]\t%d\t%lf\tMbps\n", size, avr / 1024.0);
            throughput = GetThroughput(sockfd, size, buffer);
            sum += throughput;
            avrnew = sum / ++j;
            //printf("[debug-new]\t%d\t%lf\tMbps\n", size, avrnew / 1024.0);
            if ( fabs(avrnew - avr) / avr < 0.01) {
                //printf("avrnew %lf avr %lf\n, avrnew - avr %lf\n", avrnew, avr, fabs(avrnew-avr));
                
                break;
            }
            avr = avrnew;
        } while (1);
        avr = avrnew;
        printf("%d\t%lf\tMbps\n", size, avr / 1024.0);
        

    }
    close(sockfd);

    return 0;
}


double GetThroughput(int fd, int size, char* buffer)
{
    char buff[100];
    double secDiff;
    int n;
    double throughput;
    struct timeval t, tend;

    memset(buff, 0, sizeof(buff));
    gettimeofday(&t, NULL);
    sizeSend(fd, buffer, BUFFERSIZE, 0, size);
    
    n = recv(fd, buff, 5, 0);
    if (strcmp(buff, "done") == 0) {
        gettimeofday(&tend, NULL);
        secDiff = (tend.tv_sec - t.tv_sec) * 1000000 + tend.tv_usec - t.tv_usec;
        /* Here we minus the "done" overhead. */
        secDiff -= rtt;
        
        throughput = ((double)BUFFERSIZE / (double)secDiff) * (double)1000000 * 8.0 / 1024;
        //printf("%lf ms, throughput = %lf Mbps = %lf MB/s\n", (double)secDiff, throughput, throughput / 8.0 / 1024);
        //printf("%lf\tMbps\n", throughput / 1024.0 );
    }
    else {
        fprintf(stderr, "[panic] what is server doing?\n");
        exit(1);
    }
    return throughput;
}



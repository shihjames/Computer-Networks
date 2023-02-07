#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* simple client, takes two parameters, the server domain name, and the server port number */

int main(int argc, char **argv)
{
    /* our client socket */
    int sock;

    /* variables for identifying the server */
    unsigned int server_addr;
    struct sockaddr_in sin;
    struct addrinfo *getaddrinfo_result, hints;

    /* convert server domain name to IP address */
    memset(&hints, 0, sizeof(struct addrinfo));
    /* indicates we want IPv4 */
    hints.ai_family = AF_INET;

    if (getaddrinfo(argv[1], NULL, &hints, &getaddrinfo_result) == 0)
    {
        server_addr = (unsigned int)((struct sockaddr_in *)(getaddrinfo_result->ai_addr))->sin_addr.s_addr;
        freeaddrinfo(getaddrinfo_result);
    }

    /* server port number */
    unsigned short server_port = atoi(argv[2]);

    char *buffer, *sendbuffer;
    int size = atoi(argv[3]);
    int count;
    int num = atoi(argv[4]);

    /* allocate a memory buffer in the heap */
    /*
    putting a buffer on the stack like:

    char buffer[500];

    leaves the potential for
    buffer overflow vulnerability
    */
    buffer = (char *)malloc(size);
    if (!buffer)
    {
        perror("failed to allocated buffer");
        abort();
    }

    sendbuffer = (char *)malloc(size);
    if (!sendbuffer)
    {
        perror("failed to allocated sendbuffer");
        abort();
    }
    else
    {
        /* set the default message*/
        unsigned short *size_msg = (unsigned short *)sendbuffer;
        *size_msg = size;

        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct timeval *tv_ptr = (struct timeval *)(sendbuffer + 2);
        *tv_ptr = tv;

        char *write_ptr = sendbuffer + 18;
        int i = 0;
        for (i = 0; i < size - 18; i++)
        {
            write_ptr[i] = 'a';
        }
    }

    /* create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("opening TCP socket");
        abort();
    }

    /* fill in the server's address */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(server_port);

    /* connect to the server */
    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("connect to server failed");
        abort();
    }

    /* everything looks good, since we are expecting a
       message from the server in this example, let's try receiving a
       message from the socket. this call will block until some data
       has been received */

    while (num)
    {
        /* write the timestamp into the buffer*/
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct timeval *tv_ptr = (struct timeval *)(sendbuffer + 2);
        *tv_ptr = tv;

        // send the data the server
        if (send(sock, sendbuffer, size, 0) < 0)
        {
            printf("Send fail");
            continue;
        }

        // record the received size
        int recvSize = 0;
        while ((count = recv(sock, buffer, size, 0)) >= 0 && recvSize + count < size)
        {
            recvSize += count;
        }

        if (count < 0)
        {
            perror("receive failure");
            continue;
        }
        else
        {
            struct timeval endtime;
            gettimeofday(&endtime, NULL);

            double sec_diff = (double)endtime.tv_sec - (double)tv.tv_sec;
            double usec_diff = (double)endtime.tv_usec - (double)tv.tv_usec;
            double diff = (sec_diff * 1000000 + usec_diff) / 1000;

            printf("Here is the RTT: %.3f ms\n", diff);
        }

        num--;
    }

    return 0;
}
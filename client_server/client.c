#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// number of bytes in the TCP payload
#define PAYLOAD 1024

// number of payloads to send
#define TOTAL 75000

int send_data(const char* ipaddr, int port, int size)
{
    printf("send_data\n");
    int fd;
    int n;
    char data[PAYLOAD] = {0};
    struct sockaddr_in server;
    // struct sockaddr_in client;

    printf("Start %s\n", ipaddr);

    fd                     = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipaddr);
    server.sin_port        = htons(port);

    connect(fd, (struct sockaddr*)&server, sizeof(server));
    // 15625 packets to send

    int i = 0;
    for(; i < size; ++i)
    {
        n = sendto(fd, data, PAYLOAD, 0, (struct sockaddr*)&server, sizeof(server));
        if(n < 0)
        {
            perror("Error sending data");
        }
    }
    printf("Done %s\n", ipaddr);
    close(fd);
    return 0;
}

int recv_data(const char* ipaddr, int port, int size)
{
    printf("recv_data\n");
    int fd;
    int n;
    char data[PAYLOAD] = {0};
    struct sockaddr_in server;
    socklen_t server_len;
    // struct sockaddr_in client;

    printf("Start %s\n", ipaddr);

    fd                     = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipaddr);
    server.sin_port        = htons(port);
    server_len             = sizeof(server);

    connect(fd, (struct sockaddr*)&server, server_len);
    // 15625 packets to send

    int i = 0;
    for(; i < size; ++i)
    {
        // n = recvfrom(fd, data, sizeof(data), 0, struct sockaddr *restrict __addr, socklen_t *restrict __addr_len)
        n = recvfrom(fd, data, PAYLOAD, 0, (struct sockaddr*)&server, &server_len);
        if(n < 0)
        {
            perror("Error receiving data");
            exit(-1);
        }
    }
    printf("Done %s\n", ipaddr);
    close(fd);
    return 0;
}


int main(int argc, char* argv[])
{
    int (*func)(const char*, int, int);
    func = argc > 2 ? recv_data : send_data;

    if(fork() != 0)
    {
        sleep(3);
        func(argv[1], 9876, TOTAL);
    }
    else
    {
        func(argv[1], 48698, TOTAL);
    }

    return 0;
}

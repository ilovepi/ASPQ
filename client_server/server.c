#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

int receiver(int port)
{
    printf("Receiver\n");
    int connfd, listenfd, n;
    struct sockaddr_in server = {0};
    struct sockaddr_in client = {0};
    socklen_t client_len;
    pid_t childpid;
    char msg[1500];

    listenfd               = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    bind(listenfd, (struct sockaddr*)&server, sizeof(server));

    listen(listenfd, 1024);

    for(;;)
    {
        client_len = sizeof(client);
        connfd     = accept(listenfd, (struct sockaddr*)&client, &client_len);

        if((childpid = fork()) == 0)
        {
            for(;;)
            {
                n = recvfrom(connfd, msg, 1500, 0, (struct sockaddr*)&client, &client_len);
                if(n < 0)
                {
                    if(errno == ECONNRESET)
                    {
                        close(connfd);
                        return 0;
                    }
                    perror("Problem receiving data");
                    exit(-1);
                }
            }
        }
        close(connfd);
    }
    close(listenfd);

    return 0;
}


int sender(int port)
{
    printf("Sender\n");
    int connfd, listenfd, n;
    struct sockaddr_in server = {0};
    struct sockaddr_in client = {0};
    socklen_t client_len;
    pid_t childpid;
    char msg[1500];

    listenfd               = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    bind(listenfd, (struct sockaddr*)&server, sizeof(server));

    listen(listenfd, 1024);

    for(;;)
    {
        client_len = sizeof(client);
        connfd     = accept(listenfd, (struct sockaddr*)&client, &client_len);

        if((childpid = fork()) == 0)
        {
            for(;;)
            {
                n = sendto(connfd, msg, 1500, 0, (struct sockaddr*)&client, client_len);
                if(n < 0)
                {
                    if(errno == ECONNRESET)
                    {
                        perror("connection closed normally... probably");
                        close(connfd);
                        return 0;
                    }
                    perror("Problem receiving data");
                    exit(-1);
                }
            }
        }
        close(connfd);
    }
    close(listenfd);

    return 0;
}


int main(int argc, char* argv[])
{

    int (*func)(int);
    func        = argc > 1 ? sender : receiver;
    pid_t child = fork();
    if(child == 0)
    {
        func(9876);
        prctl(PR_SET_PDEATHSIG, SIGHUP);
    }
    else
    {
        func(48698);
    }
}

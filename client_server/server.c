#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>

int receiver(int port)
{
    int connfd, listenfd, n;
    struct sockaddr_in server = {0};
    struct sockaddr_in client = {0};
    socklen_t client_len;
    pid_t childpid;
    char msg[1500];

    listenfd=socket(AF_INET, SOCK_STREAM,0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr= htonl(INADDR_ANY);
    server.sin_port= htons(port);
    bind(listenfd, (struct sockaddr*)&server, sizeof(server));

    listen(listenfd, 1024);

    for(;;)
    {
         client_len = sizeof(client);
         connfd = accept(listenfd, (struct sockaddr*)&client, &client_len);

         if((childpid = fork())==0)
         {
              for(;;)
              {
                   n = recvfrom(connfd, msg, 1500, 0, (struct sockaddr*)&client, &client_len);
                   if(n < 0){
                       perror("Problem receiving data");
                       break;
                   }
              }
         }
         close(connfd);
    }
    close(listenfd);

    return 0;
}


int main()
{
    pid_t child = fork();
    if(child ==0)
    {
        receiver(9876);
        prctl(PR_SET_PDEATHSIG, SIGHUP);
    }
    else
    {
         receiver(48698);
    }

}

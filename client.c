#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SIZE 256 
#define TOTAL 2048 

int send_data(int port)
{
	int fd;
	int n;
	char data[SIZE] = {0};
	struct sockaddr_in server;
	struct sockaddr_in client;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("169.254.8.236");
	server.sin_port = htons(port);

	connect(fd, (struct sockaddr*) &server, sizeof(server));
	//15625 packets to send

	int i = 0;
	for(;i < TOTAL; ++i)
	{
		n = sendto(fd, data, SIZE, 0, (struct sockaddr*)&server, sizeof(server));
		if(n < 0)
		{
			perror("Error sending data");
		}
	}
	close(fd);
	return 0;
}



int main()
{
	if(fork() == 0)
	{
		sleep(3);
		send_data(9876);
	}
	else
	{
		send_data(48698);
	}
	return 0;
}


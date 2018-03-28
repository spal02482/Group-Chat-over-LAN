#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <poll.h>

#define BUFF_SIZE 150
#define TIMEOUT 500

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		printf("error : not enough argument\n");
		exit(1);
	}

	int status;
	struct addrinfo hints, *res;
	/* 
	 * pollfd structure stores descriptors and events associaed with them.
	 * here one is for client's stdin and one for client's socket
	 */
	struct pollfd fds[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) 
	{
		perror(gai_strerror(status));
	}

	if ((fds[1].fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
	{
		perror("socket");
	}

	fds[1].events = POLLIN;

	if (connect(fds[1].fd, res->ai_addr, res->ai_addrlen) == -1)
	{
		perror("connect");
	}

	while (1)
       	{
		if (poll(fds, 2, TIMEOUT) > 0)
		{
			char buff[BUFF_SIZE];
			size_t send_bytes, recv_bytes;
			memset(buff, 0, sizeof(buff));
			/* event : client receives data on stdin */
			if (fds[0].revents && POLLIN)
			{
				fgets(buff, BUFF_SIZE, stdin);
				if ((send_bytes = send(fds[1].fd, buff, strlen(buff), 0)) != strlen(buff))
				{
					perror("send");
				}
			}
			/* event : client receives data on its socket */
			else if (fds[1].revents && POLLIN)
			{
				if ((recv_bytes = recv(fds[1].fd, buff, BUFF_SIZE, 0)) != 0)
				{
					buff[recv_bytes] = '\0';
					fputs(buff, stdout);
				}
				/* if server has closed the connection, then close the file descriptor */
				else
				{
					close(fds[1].fd);
				}
			}
	
		}
	}
	return 0;
}

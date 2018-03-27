#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string>
using std::string;

#define BUFF_SIZE 150
#define BACKLOG 5

bool send_broadcast(struct pollfd fds[], int fds_index, string ip, char *buff, int sender_index)
{
	size_t len_buff = strlen(buff), len_ip = ip.length(), send_bytes;
	bool sent_to_all = true;

	char msg[150];
	ip.copy(msg, len_ip);
	strcpy(msg + len_ip, " : ");
	strcpy(msg + len_ip + 3, buff);
	size_t len_msg = len_ip + len_buff + 3;

	for (int i = 2; i < fds_index; i++)
	{ 
		if (sender_index == i)
		{
			continue;
		}

		if ((send_bytes = send(fds[i].fd, msg, len_msg, 0)) != len_msg)
		{
			perror("send");
			sent_to_all = false;
		}	
	}

	return sent_to_all;
}
	 
void delete_client(struct pollfd fds[], int client_index, int *fds_index, std::vector< string > fds_address)
{
	close(fds[client_index].fd);
	for (int i = client_index; i < *fds_index; i++)
	{
		fds[i] = fds[i + 1];
	}

	*fds_index -= 1;
	fds_address.erase(fds_address.begin() + client_index - 1);
}

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		printf("error : not enough argument\n");
		exit(1);
	}

	struct addrinfo hints, *res;
	struct sockaddr_storage s_stg[10];
	int status, sfd, cfd[10];
	socklen_t size = sizeof(struct sockaddr_storage);
	struct pollfd fds[12];
	int fds_index = 2;
	std::vector< string > fds_address(12);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
 
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	fds_address[0] = argv[1];
	fds_address[1] = argv[1];

	if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0)
	{
		perror(gai_strerror(status));
	}

	if ((fds[1].fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
	{
		perror("socket");
	}
	fds[1].events = POLLIN;

	if (bind(fds[1].fd, res->ai_addr, sizeof(struct sockaddr_in)) != 0)
	{
		perror("bind");
	}

	if (listen(fds[1].fd, BACKLOG) != 0)
	{
		perror("listen");
		
	}

	while (1)
	{
		char buff[100], ip[16];
		memset(buff, 0, sizeof(buff));
		memset(ip, 0 , sizeof(ip));
		if (poll(fds, fds_index, 500) > 0)
		{
			if (fds[0].revents && POLLIN)
			{
				fgets(buff, 100, stdin);
				send_broadcast(fds, fds_index, fds_address[0], buff, 0);
			}
			else if (fds[1].revents && POLLIN)
			{
				if ((fds[fds_index].fd = accept(fds[1].fd, (struct sockaddr *)(&s_stg[fds_index]), &size)) < 0)
				{
					perror("accept");
				}
				else
				{
					inet_ntop(AF_INET, &(((struct sockaddr_in *)(&s_stg[fds_index]))->sin_addr), ip, sizeof(ip));
					printf("%s has joined conversation.\n", ip);
					fds_address[fds_index] = ip;
					strcpy(buff, "has joined conversation.\n");
					fds[fds_index].events = POLLIN;
					fds_index++;
					send_broadcast(fds, fds_index, fds_address[fds_index - 1], buff, fds_index - 1);
				}
			}
			else
			{
				for (int i = 2; i < fds_index; i++)
				{
					if (fds[i].revents && POLLIN)
					{
						int recv_bytes = recv(fds[i].fd, buff, 100, 0);
						buff[recv_bytes] = '\0';
						if (recv_bytes != 0)
						{
							printf("(..%d) %s : %s", i, fds_address[i].c_str(), buff);
							send_broadcast(fds, fds_index, fds_address[i], buff, i);
						}
						else
						{
							delete_client(fds, i, &fds_index, fds_address);
						}					
					}
				}
			}
		}
	}
	return 0;
}

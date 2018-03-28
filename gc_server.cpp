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
#define TIMEOUT 500
#define CONN 10
/*
 * A function which will send the given message to all the clients present in the LAN, except
 * the user which has sent it, to avoid the duplication of the messages at the sender.
 * Return type : A boolean variable which is true if message has been sent to all other client, otherwise false.
 */

bool send_broadcast(struct pollfd fds[], int fds_index, string ip, char *buff, int sender_index)
{
	size_t len_buff = strlen(buff), len_ip = ip.length(), send_bytes;

	bool sent_to_all = true;

	char msg[BUFF_SIZE];

	/* copy the IP and the message to the msg buffer */
	ip.copy(msg, len_ip);
	strcpy(msg + len_ip, " : ");
	strcpy(msg + len_ip + 3, buff);
	size_t len_msg = len_ip + len_buff + 3;

	/* send the message to all the clients */
	for (int i = 2; i < fds_index; i++)
	{ 
		/* don't send it to the sender which has sent it */
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

/*
 * A function to delete the pollfd structure for the client if the connection is closed from the client side
 * either by process termination or closing the file descriptor.
 */
	 
void delete_client(struct pollfd fds[], int client_index, int *fds_index, std::vector< string > fds_address)
{
	/* close the socket and delete the pollfd structure associated with that socket */
	close(fds[client_index].fd);
	for (int i = client_index; i < *fds_index; i++)
	{
		fds[i] = fds[i + 1];
	}

	/* decrement the total number of clients and delete the address associated with the client */
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

	/*
	 * sockaddr_storage structure to store the client information when accepting
	 * the connections using accept() system call.
	 */
	struct sockaddr_storage s_stg[CONN];

	int status, sfd;
	socklen_t size = sizeof(struct sockaddr_storage);

	/* 
	 * pollfd structure which will store the socket descriptor and associated events with them. 
	 * 2 additional descriptors are there, one is for the server's stdin and one for server's listening socket.
	 */
	struct pollfd fds[CONN + 2];

	/* variable to store the total number of descriptors including server's stdin and listening socket */
	int fds_index = 2;
	
	/* vector to store the addresses of the clients */
	std::vector< string > fds_address(CONN + 2);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
 
	/* set 0th descriptor as stdin and set the read event */
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;


	fds_address[0] = argv[1];
	fds_address[1] = argv[1];

	if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0)
	{
		perror(gai_strerror(status));
	}

	/* create the listening socket and set read event */
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
		char buff[BUFF_SIZE], ip[16];
		memset(buff, 0, sizeof(buff));
		memset(ip, 0 , sizeof(ip));

		if (poll(fds, fds_index, TIMEOUT) > 0)
		{
			/* event : server receives data on stdin */
			if (fds[0].revents && POLLIN)
			{
				/* read and send the message to all the clients */
				fgets(buff, BUFF_SIZE, stdin);
				send_broadcast(fds, fds_index, fds_address[0], buff, 0);
			}
			/* event : server waits for incoming connection on listening socket */
			else if (fds[1].revents && POLLIN)
			{
				if ((fds[fds_index].fd = accept(fds[1].fd, (struct sockaddr *)(&s_stg[fds_index]), &size)) < 0)
				{
					perror("accept");
				}
				else
				{
					/* get the ip address of the client using inet_ntop */
					inet_ntop(AF_INET, &(((struct sockaddr_in *)(&s_stg[fds_index]))->sin_addr), ip, sizeof(ip));
					
					printf("%s has joined conversation.\n", ip);
					/* store the client's ip */
					fds_address[fds_index] = ip;
					strcpy(buff, "has joined conversation.\n");
					send_broadcast(fds, fds_index, fds_address[fds_index], buff, fds_index);

					/* set the read event and incremet fds_index */
					fds[fds_index].events = POLLIN;
					fds_index++;
				}
			}
			/* event : server receives messages from one of it's clients */
			else
			{
				for (int i = 2; i < fds_index; i++)
				{
					if (fds[i].revents && POLLIN)
					{
						int recv_bytes = recv(fds[i].fd, buff, BUFF_SIZE, 0);
						buff[recv_bytes] = '\0';
						/* print the message and send it to all other clients */
						if (recv_bytes != 0)
						{
							printf("(..%d) %s : %s", i, fds_address[i].c_str(), buff);
							send_broadcast(fds, fds_index, fds_address[i], buff, i);
						}
						/* if client closes the connection then delete the client */
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

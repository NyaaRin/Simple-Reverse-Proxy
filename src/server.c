#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "headers/main.h"
#include "headers/server.h"
#include "headers/remote.h"
#include "headers/poll.h"
#include "headers/connection.h"
#include "headers/utils.h"

void server_bind(int socket, uint32_t local_addr, uint16_t local_port)
{
	struct sockaddr_in addr;
	util_zero(&addr, 16);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = local_addr;
	addr.sin_port = local_port;
	if(bind(socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		return;
	}
	printf("Bound to local address on port %d\r\n", ntohs(local_port));
	return;
}

void *server_start(void *vsettings)
{
	pthread_detach(pthread_self());
	struct program_settings_t *settings = vsettings;
	connection_init(settings->listening_sock);
	listen(settings->listening_sock, 30);
	while(1)
	{
		socklen_t addr_len = sizeof(struct sockaddr_in);
		struct sockaddr_in addr;
		util_zero(&addr, sizeof(addr));
		int sockfd = accept(settings->listening_sock, (struct sockaddr *)&(addr), &addr_len);
		if(sockfd < 0)
		{
			printf("Server Error!\r\n");
		}
		//
		// make a remote connection
		//
		struct connection_t *conn = connection_add(sockfd);
		conn->rsid = socket(AF_INET, SOCK_STREAM, 0);
		if(remote_connect(conn->rsid) == 0)
		{
			send(sockfd, "Failed to connect!\r\n", 20, MSG_NOSIGNAL);
			close(sockfd);
			sockfd = -1;
			printf("Failed to connect to remote host!\r\n");
			continue;
		}
		poll_addqueue(conn);
		continue;
	}
	pthread_exit(NULL);
}

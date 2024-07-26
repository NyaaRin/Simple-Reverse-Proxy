#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#include "headers/main.h"
#include "headers/connection.h"
#include "headers/utils.h"

struct connection_t **connections = NULL;
size_t connections_len = 0;

struct connection_t *connection_add(int sid)
{
	connections = realloc(connections, (connections_len+1)*sizeof(struct connection_t *));
	connections[connections_len] = malloc(sizeof(struct connection_t));
	util_zero(&(connections[connections_len]->tid), sizeof(pthread_t));
	connections[connections_len]->sid = sid;
	connections[connections_len]->rsid = 0;
	connections[connections_len]->state = CON_STATE_ACCEPT;
	connections[connections_len]->outbuffer = NULL;
	connections[connections_len]->outbufferLen = 0;
	struct connection_t *ret = connections[connections_len];
	connections_len += 1;
	return ret;
}

void connection_init(int accept_fd)
{
	connection_add(accept_fd);
	return;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "headers/main.h"
#include "headers/poll.h"
#include "headers/connection.h"
#include "headers/utils.h"

struct poll_t 
{
	struct connection_t *conn;
};
struct poll_t **queued_list = {NULL};
size_t queued_list_len = 0;
pthread_mutex_t queue_lock;

void poll_init(void)
{
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&queue_lock, &Attr);
}

void poll_addqueue(struct connection_t *conn)
{
	pthread_mutex_lock(&queue_lock);
	
	if(queued_list_len > 0)
	{
		unsigned char exists = 0;
		size_t i;
		for(i = 0; i < queued_list_len; i++)
		{
			struct connection_t *active_con = queued_list[i]->conn;
			if(active_con->sid == conn->sid)
			{
				exists = 1;
			}
			if(active_con->rsid == conn->rsid)
			{
				exists = 1;
			}
		}
		if(exists == 1)
		{
			printf("[poll_addqueue] Connection FD already exists!\r\n");
			pthread_mutex_unlock(&queue_lock);
			return;
		}
	}
	
	queued_list = realloc(queued_list, (queued_list_len+1) * sizeof(struct poll_t *));
	queued_list[queued_list_len] = malloc(sizeof(struct poll_t));
	util_zero(queued_list[queued_list_len], sizeof(struct poll_t));
	queued_list[queued_list_len]->conn = conn;
	queued_list_len += 1;
	
	pthread_mutex_unlock(&queue_lock);
	return;
}

void poll_delqueue(struct connection_t *conn)
{
	pthread_mutex_lock(&queue_lock);
	
	if(queued_list_len > 0)
	{
		unsigned char exists = 0;
		do
		{
			exists = 0;
			size_t i = 0;
			for(i = 0; i < queued_list_len; i++)
			{
				struct connection_t *active_con = queued_list[i]->conn;
				if(active_con->sid == conn->sid)
				{
					exists = 1;
				}
				if(active_con->rsid == conn->rsid)
				{
					exists = 1;
				}
				if(exists == 1)
				{
					free(queued_list[i]);
					queued_list[i] = NULL;
					size_t x = 0;
					for(x = i; x < queued_list_len-1; x++)
					{
						queued_list[x] = queued_list[x+1];
						continue;
					}
					queued_list_len -= 1;
					queued_list = realloc(queued_list, (queued_list_len)*sizeof(struct poll_t *));
				}
			}
		} while(exists == 1);
	}

	pthread_mutex_unlock(&queue_lock);
	return;
}

void *poll_process(void *useless)
{
	pthread_detach(pthread_self());
	while(1)
	{
		pthread_mutex_lock(&queue_lock);
		size_t i;
		for(i = 0; i < queued_list_len; i++)
		{
			if(queued_list[i]->conn->state == CON_STATE_RECV)
			{
				// run (recv from sid | send to rsid)
				send(queued_list[i]->conn->rsid, queued_list[i]->conn->outbuffer, queued_list[i]->conn->outbufferLen, MSG_NOSIGNAL);
				queued_list[i]->conn->state = CON_STATE_PROC;
				continue;
			}
			if(queued_list[i]->conn->state == CON_STATE_SEND)
			{
				// run (recv from rsid | send to sid)
				send(queued_list[i]->conn->sid, queued_list[i]->conn->outbuffer, queued_list[i]->conn->outbufferLen, MSG_NOSIGNAL);
				queued_list[i]->conn->state = CON_STATE_PROC;
				continue;
			}
			unsigned char remote_ready = 0;
			unsigned char client_ready = 0;
			struct timeval timeout;
			fd_set master_set, working_set, write_set;
			int rc = -1, max_sd = -1;
			FD_ZERO(&master_set);
			FD_ZERO(&write_set);
			if(queued_list[i]->conn->rsid > queued_list[i]->conn->sid)
			{
				max_sd = queued_list[i]->conn->rsid;
			}
			else
			{
				max_sd = queued_list[i]->conn->sid;
			}
			FD_SET(queued_list[i]->conn->sid, &master_set);
			if(queued_list[i]->conn->state == CON_STATE_ACCEPT)
			{
				FD_SET(queued_list[i]->conn->rsid, &write_set);
			}
			else
			{
				FD_SET(queued_list[i]->conn->rsid, &master_set);
			}
			timeout.tv_sec  = 0;
			timeout.tv_usec = 600;
			util_cpy(&working_set, &master_set, sizeof(fd_set));
			rc = select(max_sd + 1, &working_set, &write_set, NULL, &timeout);
			if (rc > 0)
			{
				if(FD_ISSET(queued_list[i]->conn->sid, &working_set))
				{
					client_ready = 1;
				}
				if(FD_ISSET(queued_list[i]->conn->rsid, &working_set) || FD_ISSET(queued_list[i]->conn->rsid, &write_set))
				{
					remote_ready = 1;
				}
			}
			else
			{
				continue;
			}
			
			if(remote_ready == 1 && queued_list[i]->conn->state == CON_STATE_ACCEPT)
			{
				socklen_t lon = sizeof(int);
				int valopt;
				getsockopt(queued_list[i]->conn->rsid, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
				if (valopt != 0) 
				{
					close(queued_list[i]->conn->rsid);
					close(queued_list[i]->conn->sid);
					poll_delqueue(queued_list[i]->conn);
				} 
				else 
				{
					long flags = fcntl(queued_list[i]->conn->rsid, F_GETFL, NULL);
					flags &= (~O_NONBLOCK);
					fcntl(queued_list[i]->conn->rsid, F_SETFL, flags);
					queued_list[i]->conn->state = CON_STATE_PROC;
				}
				continue;
			}
			if(queued_list[i]->conn->state == CON_STATE_ACCEPT)
			{
				continue;
			}
			if(queued_list[i]->conn->state == CON_STATE_PROC)
			{
				if(client_ready == 1)
				{
					unsigned char tmp[4096];
					util_zero(tmp, 4096);
					int rc = recv(queued_list[i]->conn->sid, tmp, 4096, MSG_NOSIGNAL | MSG_PEEK);
					if(rc < 1)
					{
						//connection closed
						close(queued_list[i]->conn->rsid);
						close(queued_list[i]->conn->sid);
						poll_delqueue(queued_list[i]->conn);
						continue;
					}
					queued_list[i]->conn->outbuffer = malloc(rc+1);
					queued_list[i]->conn->outbufferLen = rc;
					util_zero(queued_list[i]->conn->outbuffer, rc+1);
					recv(queued_list[i]->conn->sid, queued_list[i]->conn->outbuffer, rc, MSG_NOSIGNAL);
					queued_list[i]->conn->state = CON_STATE_RECV;
					continue;
				}
				if(remote_ready == 1)
				{
					unsigned char tmp[4096];
					util_zero(tmp, 4096);
					int rc = recv(queued_list[i]->conn->rsid, tmp, 4096, MSG_NOSIGNAL | MSG_PEEK);
					if(rc < 1)
					{
						//connection closed
						close(queued_list[i]->conn->rsid);
						close(queued_list[i]->conn->sid);
						poll_delqueue(queued_list[i]->conn);
						continue;
					}
					queued_list[i]->conn->outbuffer = malloc(rc+1);
					queued_list[i]->conn->outbufferLen = rc;
					util_zero(queued_list[i]->conn->outbuffer, rc+1);
					recv(queued_list[i]->conn->rsid, queued_list[i]->conn->outbuffer, rc, MSG_NOSIGNAL);
					queued_list[i]->conn->state = CON_STATE_SEND;
					continue;
				}
				continue;
			}
		}
		pthread_mutex_unlock(&queue_lock);
	}
	pthread_exit(NULL);
}


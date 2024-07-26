#pragma once

enum constate_e
{
	CON_STATE_ACCEPT,
	CON_STATE_RECV,
	CON_STATE_PROC,
	CON_STATE_SEND,
	CON_STATE_HALT
};

struct connection_t
{
	pthread_t tid;
	int sid;
	int rsid;
	uint8_t state;
	unsigned char *outbuffer;
	size_t outbufferLen;
};

struct connection_t *connection_add(int sid);
void connection_init(int accept_fd);

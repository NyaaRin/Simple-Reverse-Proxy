#pragma once
#include <stdint.h>

struct program_settings_t
{
	uint32_t remote_host;
	uint32_t local_host;
	uint16_t remote_port;
	uint16_t local_port;
	int listening_sock;
};

extern struct program_settings_t settings;

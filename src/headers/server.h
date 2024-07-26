#pragma once
#include <stdint.h>
void server_bind(int socket, uint32_t local_addr, uint16_t local_port);
void *server_start(void *vsettings);


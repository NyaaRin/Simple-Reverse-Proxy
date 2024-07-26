#pragma once
#include <stdint.h>
#include "connection.h"
void poll_init(void);
void poll_addqueue(struct connection_t *conn);
void poll_delqueue(struct connection_t *conn);
void *poll_process(void *useless);
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "headers/main.h"
#include "headers/utils.h"

void util_zero(void *ptr, size_t size)
{
	uint8_t *bytes = ptr;
	size_t x = 0;
	for(x = 0; x < size; x++)
	{
		bytes[x] = 0x00;
	}
	return;
}

void util_cpy(void *dst, void *src, size_t sz)
{
	uint8_t *dst_byt = dst;
	uint8_t *src_byt = src;
	size_t x = 0;
	
	for(x = 0; x < sz; x++)
	{
		dst_byt[x] = src_byt[x];
		continue;
	}
	
	return;
}

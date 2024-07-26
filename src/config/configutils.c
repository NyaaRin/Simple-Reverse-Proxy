#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "configutils.h"

void configutil_zero(void *ptr, int size)
{
    uint8_t *ptr_p = ptr; 
    int j;
    for(j = 0; j < size; j++)
    {
        ptr_p[j] = 0;
    }
    return;
}

int configutil_len(void *ptr)
{
    uint8_t *ptr_p = ptr; 
    int ret = 0;
    while(ptr_p[ret] != 0)
    {
        ret++;
    }
    return ret;
}

int configutil_cpy(void *to_ptr, void *ptr, int start, int stop)
{
    uint8_t *ptr_p = ptr; 
    uint8_t *to_ptr_p = to_ptr; 
    int from_pos = 0;
    int j;
    for(j = start; j < stop; j++)
    {
        #ifdef DEBUG
        //printf("BEFORE to_ptr[%d] == '%c' && ptr[%d] == '%c'\r\n", j, to_ptr_p[j], from_pos, ptr_p[from_pos]);
        #endif
        to_ptr_p[j] = ptr_p[from_pos];
        #ifdef DEBUG
        //printf("AFTER to_ptr[%d] == '%c' && ptr[%d] == '%c'\r\n", j, to_ptr_p[j], from_pos, ptr_p[from_pos]);
        #endif
        from_pos++;
    }
    return stop;
}

uint8_t **configutil_tokenize(const uint8_t *buf, const int buf_size, int *count, const uint8_t delim)
{
    uint8_t **ret = NULL;
    int ret_count = 0;
    int token_pos = 0;
    int pos = 0;
    uint8_t token[4096];

    if(buf_size > 4096) return NULL;

    configutil_zero(token, 4096);
    for (pos = 0; pos < buf_size; pos++)
    {
        if(buf[pos] == delim)
        {
            token[token_pos] = 0;
            
            ret = realloc(ret, (ret_count + 1) *sizeof(uint8_t *));
            ret[ret_count] = malloc(token_pos + 1);
            configutil_zero(ret[ret_count], token_pos+1);
            configutil_cpy(ret[ret_count], token, 0, token_pos);
            ret_count++;

            configutil_zero(token, 4096);
            token_pos = 0;
        }
        else
        {
            token[token_pos] = buf[pos];
            token_pos++;
        }
    }

    if(token_pos > 0)
    {
        if(ret_count == 0) ret = NULL;
        ret = realloc(ret, (ret_count + 1) *sizeof(uint8_t *));
        ret[ret_count] = malloc(token_pos + 1);
        configutil_zero(ret[ret_count], token_pos+1);
        configutil_cpy(ret[ret_count], token, 0, token_pos);
        ret_count++;

        configutil_zero(token, 4096);
        token_pos = 0;
    }

    *count = ret_count;

    configutil_zero(token, 4096);

    if(ret_count > 0) return ret;
    return NULL;
}

int configutil_get_white_end(void *ptr, int size)
{
    uint8_t *ptr_p = ptr;
    int pos, ret = 0;
    while((ptr_p[ret]) == ' ' || ((ptr_p[ret]) == '\t'))
    {
        ret++;
    }
    return ret;
}

int configutil_get_char_count(void *ptr, char delim)
{
    uint8_t *ptr_p = ptr;
    int pos = 0, ret = 0;
    while(ptr_p[pos] != '\0')
    {
        if(ptr_p[pos] == delim) ret++;
        pos++;
    }
    return ret;
}

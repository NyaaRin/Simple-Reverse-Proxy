#pragma once

void configutil_zero(void *ptr, int size);
int configutil_len(void *ptr);
int configutil_cpy(void *to_ptr, void *ptr, int start, int stop);
uint8_t **configutil_tokenize(const uint8_t *buf, const int buf_size, int *count, const uint8_t delim);
int configutil_get_white_end(void *ptr, int size);
int configutil_get_char_count(void *ptr, char delim);

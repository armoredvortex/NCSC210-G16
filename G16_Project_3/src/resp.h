#ifndef RESP_H
#define RESP_H

#include <stddef.h>

typedef struct {
  char *ptr;
  size_t len;
} resp_buf_t;

typedef struct {
  int count;
  resp_buf_t *items;
} resp_array_t;

int resp_read_array(int fd, resp_array_t *out);
void resp_free_array(resp_array_t *arr);

int resp_reply_simple(int fd, const char *msg);
int resp_reply_error(int fd, const char *msg);
int resp_reply_bulk(int fd, const void *data, size_t len);
int resp_reply_null_bulk(int fd);
int resp_reply_integer(int fd, long long value);

#endif

#include "resp.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static ssize_t recv_exact(int fd, void *buf, size_t n) {
  size_t total = 0;
  while (total < n) {
    ssize_t r = recv(fd, (char *)buf + total, n - total, 0);
    if (r == 0) {
      return 0;
    }
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    total += (size_t)r;
  }
  return (ssize_t)total;
}

static int recv_crlf_line(int fd, char *out, size_t out_sz) {
  size_t i = 0;
  while (i + 1 < out_sz) {
    char c;
    ssize_t r = recv_exact(fd, &c, 1);
    if (r <= 0) {
      return -1;
    }

    if (c == '\r') {
      char next;
      r = recv_exact(fd, &next, 1);
      if (r <= 0 || next != '\n') {
        return -1;
      }
      out[i] = '\0';
      return 0;
    }

    out[i++] = c;
  }
  return -1;
}

static int parse_int(const char *s, int *out) {
  char *end = NULL;
  errno = 0;
  long v = strtol(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0' || v < INT_MIN || v > INT_MAX) {
    return -1;
  }
  *out = (int)v;
  return 0;
}

void resp_free_array(resp_array_t *arr) {
  if (arr == NULL) {
    return;
  }
  for (int i = 0; i < arr->count; ++i) {
    free(arr->items[i].ptr);
  }
  free(arr->items);
  arr->items = NULL;
  arr->count = 0;
}

int resp_read_array(int fd, resp_array_t *out) {
  char lead;
  if (recv_exact(fd, &lead, 1) <= 0) {
    return -1;
  }

  if (lead != '*') {
    return -1;
  }

  char line[64];
  if (recv_crlf_line(fd, line, sizeof(line)) != 0) {
    return -1;
  }

  int nitems = 0;
  if (parse_int(line, &nitems) != 0 || nitems <= 0 || nitems > 128) {
    return -1;
  }

  out->count = nitems;
  out->items = calloc((size_t)nitems, sizeof(resp_buf_t));
  if (out->items == NULL) {
    return -1;
  }

  for (int i = 0; i < nitems; ++i) {
    char kind;
    if (recv_exact(fd, &kind, 1) <= 0 || kind != '$') {
      resp_free_array(out);
      return -1;
    }

    if (recv_crlf_line(fd, line, sizeof(line)) != 0) {
      resp_free_array(out);
      return -1;
    }

    int blen = 0;
    if (parse_int(line, &blen) != 0 || blen < 0) {
      resp_free_array(out);
      return -1;
    }

    out->items[i].len = (size_t)blen;
    out->items[i].ptr = malloc((size_t)blen + 1);
    if (out->items[i].ptr == NULL) {
      resp_free_array(out);
      return -1;
    }

    if (recv_exact(fd, out->items[i].ptr, (size_t)blen) <= 0) {
      resp_free_array(out);
      return -1;
    }
    out->items[i].ptr[blen] = '\0';

    char crlf[2];
    if (recv_exact(fd, crlf, 2) <= 0 || crlf[0] != '\r' || crlf[1] != '\n') {
      resp_free_array(out);
      return -1;
    }
  }

  return 0;
}

static int send_all(int fd, const void *buf, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t w = send(fd, (const char *)buf + sent, len - sent, 0);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    sent += (size_t)w;
  }
  return 0;
}

int resp_reply_simple(int fd, const char *msg) {
  char line[256];
  int n = snprintf(line, sizeof(line), "+%s\r\n", msg);
  if (n < 0 || (size_t)n >= sizeof(line)) {
    return -1;
  }
  return send_all(fd, line, (size_t)n);
}

int resp_reply_error(int fd, const char *msg) {
  char line[512];
  int n = snprintf(line, sizeof(line), "-ERR %s\r\n", msg);
  if (n < 0 || (size_t)n >= sizeof(line)) {
    return -1;
  }
  return send_all(fd, line, (size_t)n);
}

int resp_reply_bulk(int fd, const void *data, size_t len) {
  char head[64];
  int n = snprintf(head, sizeof(head), "$%zu\r\n", len);
  if (n < 0 || (size_t)n >= sizeof(head)) {
    return -1;
  }

  if (send_all(fd, head, (size_t)n) != 0) {
    return -1;
  }
  if (len > 0 && send_all(fd, data, len) != 0) {
    return -1;
  }
  return send_all(fd, "\r\n", 2);
}

int resp_reply_null_bulk(int fd) {
  return send_all(fd, "$-1\r\n", 5);
}

int resp_reply_integer(int fd, long long value) {
  char line[128];
  int n = snprintf(line, sizeof(line), ":%lld\r\n", value);
  if (n < 0 || (size_t)n >= sizeof(line)) {
    return -1;
  }
  return send_all(fd, line, (size_t)n);
}

#define _POSIX_C_SOURCE 200809L

#include "file_ops.h"
#include "logger.h"
#include "path_utils.h"
#include "resp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
  int fd;
  char client[64];
} client_ctx_t;

static volatile sig_atomic_t g_stop = 0;
static char g_data_dir[PATH_MAX] = ".";

static void on_signal(int sig) {
  (void)sig;
  g_stop = 1;
}

static int parse_int_arg(const char *s, int *out) {
  char *end = NULL;
  errno = 0;
  long v = strtol(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0' || v < INT_MIN || v > INT_MAX) {
    return -1;
  }
  *out = (int)v;
  return 0;
}

static int derive_compress_dst(const char *src, char *dst, size_t dst_sz) {
  return path_append_suffix(src, ".rle", dst, dst_sz);
}

static int derive_decompress_dst(const char *src, char *dst, size_t dst_sz) {
  if (path_strip_suffix(src, ".rle", dst, dst_sz) == 0) {
    return 0;
  }
  return path_append_suffix(src, ".out", dst, dst_sz);
}

static void dispatch_command(int fd, const char *client, const resp_array_t *req) {
  const char *cmd = req->items[0].ptr;

  if (strcasecmp(cmd, "PING") == 0) {
    if (req->count == 1) {
      resp_reply_simple(fd, "PONG");
    } else {
      resp_reply_bulk(fd, req->items[1].ptr, req->items[1].len);
    }
    return;
  }

  if (strcasecmp(cmd, "GETFILE") == 0) {
    if (req->count != 2) {
      resp_reply_error(fd, "wrong number of arguments for GETFILE");
      return;
    }
    op_getfile(fd, g_data_dir, client, req->items[1].ptr);
    return;
  }

  if (strcasecmp(cmd, "SETFILE") == 0) {
    if (req->count != 3) {
      resp_reply_error(fd, "wrong number of arguments for SETFILE");
      return;
    }
    op_setfile(fd,
               g_data_dir,
               client,
               req->items[1].ptr,
               req->items[2].ptr,
               req->items[2].len);
    return;
  }

  if (strcasecmp(cmd, "DELFILE") == 0) {
    if (req->count != 2) {
      resp_reply_error(fd, "wrong number of arguments for DELFILE");
      return;
    }
    op_delfile(fd, g_data_dir, client, req->items[1].ptr);
    return;
  }

  if (strcasecmp(cmd, "RENAMEFILE") == 0) {
    if (req->count != 3) {
      resp_reply_error(fd, "wrong number of arguments for RENAMEFILE");
      return;
    }
    op_renamefile(fd, g_data_dir, client, req->items[1].ptr, req->items[2].ptr);
    return;
  }

  if (strcasecmp(cmd, "COPYFILE") == 0) {
    if (req->count != 3) {
      resp_reply_error(fd, "wrong number of arguments for COPYFILE");
      return;
    }
    op_copyfile(fd, g_data_dir, client, req->items[1].ptr, req->items[2].ptr);
    return;
  }

  if (strcasecmp(cmd, "STATFILE") == 0) {
    if (req->count != 2) {
      resp_reply_error(fd, "wrong number of arguments for STATFILE");
      return;
    }
    op_statfile(fd, g_data_dir, client, req->items[1].ptr);
    return;
  }

  if (strcasecmp(cmd, "LOGS") == 0) {
    int limit = 20;
    if (req->count == 2) {
      if (parse_int_arg(req->items[1].ptr, &limit) != 0 || limit < 0) {
        resp_reply_error(fd, "LOGS limit must be non-negative integer");
        return;
      }
    } else if (req->count != 1) {
      resp_reply_error(fd, "wrong number of arguments for LOGS");
      return;
    }
    op_logs(fd, client, limit);
    return;
  }

  if (strcasecmp(cmd, "COMPRESS") == 0) {
    char dst[PATH_MAX];
    const char *src = NULL;
    const char *out = NULL;

    if (req->count == 2) {
      src = req->items[1].ptr;
      if (derive_compress_dst(src, dst, sizeof(dst)) != 0) {
        resp_reply_error(fd, "unable to build destination path");
        return;
      }
      out = dst;
    } else if (req->count == 3) {
      src = req->items[1].ptr;
      out = req->items[2].ptr;
    } else {
      resp_reply_error(fd, "wrong number of arguments for COMPRESS");
      return;
    }

    op_compress(fd, g_data_dir, client, src, out);
    return;
  }

  if (strcasecmp(cmd, "DECOMPRESS") == 0) {
    char dst[PATH_MAX];
    const char *src = NULL;
    const char *out = NULL;

    if (req->count == 2) {
      src = req->items[1].ptr;
      if (derive_decompress_dst(src, dst, sizeof(dst)) != 0) {
        resp_reply_error(fd, "unable to build destination path");
        return;
      }
      out = dst;
    } else if (req->count == 3) {
      src = req->items[1].ptr;
      out = req->items[2].ptr;
    } else {
      resp_reply_error(fd, "wrong number of arguments for DECOMPRESS");
      return;
    }

    op_decompress(fd, g_data_dir, client, src, out);
    return;
  }

  resp_reply_error(fd, "unknown command");
}

static void *client_thread(void *arg) {
  client_ctx_t *ctx = (client_ctx_t *)arg;

  while (!g_stop) {
    resp_array_t req = {0};
    if (resp_read_array(ctx->fd, &req) != 0) {
      break;
    }

    if (req.count == 0 || req.items[0].ptr == NULL) {
      resp_free_array(&req);
      resp_reply_error(ctx->fd, "empty command");
      continue;
    }

    dispatch_command(ctx->fd, ctx->client, &req);
    resp_free_array(&req);
  }

  close(ctx->fd);
  free(ctx);
  return NULL;
}

static int start_server(uint16_t port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return -1;
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
    perror("setsockopt");
    close(server_fd);
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    perror("bind");
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, 64) != 0) {
    perror("listen");
    close(server_fd);
    return -1;
  }

  printf("filesrv listening on 0.0.0.0:%u, data dir: %s\n", port, g_data_dir);

  while (!g_stop) {
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
    int client_fd = accept(server_fd, (struct sockaddr *)&caddr, &clen);
    if (client_fd < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      break;
    }

    client_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
      close(client_fd);
      continue;
    }
    ctx->fd = client_fd;

    const char *ip = inet_ntop(AF_INET, &caddr.sin_addr, ctx->client, sizeof(ctx->client));
    if (ip == NULL) {
      (void)snprintf(ctx->client, sizeof(ctx->client), "unknown");
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_thread, ctx) != 0) {
      close(client_fd);
      free(ctx);
      continue;
    }
    pthread_detach(tid);
  }

  close(server_fd);
  return 0;
}

int main(int argc, char **argv) {
  uint16_t port = 6379;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      long p = strtol(argv[++i], NULL, 10);
      if (p <= 0 || p > 65535) {
        fprintf(stderr, "invalid port\n");
        return 1;
      }
      port = (uint16_t)p;
    } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      const char *dir = argv[++i];
      if (dir == NULL || *dir == '\0') {
        fprintf(stderr, "invalid data dir\n");
        return 1;
      }
      int n = snprintf(g_data_dir, sizeof(g_data_dir), "%s", dir);
      if (n < 0 || (size_t)n >= sizeof(g_data_dir)) {
        fprintf(stderr, "data dir too long\n");
        return 1;
      }
    } else {
      fprintf(stderr, "usage: %s [-p port] [-d data_dir]\n", argv[0]);
      return 1;
    }
  }

  if (mkdir(g_data_dir, 0755) != 0 && errno != EEXIST) {
    perror("mkdir data dir");
    return 1;
  }

  logger_init(g_data_dir);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = on_signal;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  return start_server(port) == 0 ? 0 : 1;
}

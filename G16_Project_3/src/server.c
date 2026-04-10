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

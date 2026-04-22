#include "logger.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <limits.h>

static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_log_path[PATH_MAX] = "./operations.log";

void logger_init(const char *data_dir) {
  (void)snprintf(g_log_path, sizeof(g_log_path), "%s/operations.log", data_dir);
}

const char *logger_path(void) {
  return g_log_path;
}

void logger_log(const char *client,
                const char *command,
                const char *target,
                const char *status,
                const char *detail) {
  time_t now = time(NULL);
  struct tm tmbuf;
  localtime_r(&now, &tmbuf);

  char ts[64];
  strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmbuf);

  pthread_mutex_lock(&g_log_mutex);
  FILE *fp = fopen(g_log_path, "a");
  if (fp != NULL) {
    fprintf(fp,
            "%s client=%s cmd=%s target=%s status=%s detail=%s\n",
            ts,
            client ? client : "unknown",
            command ? command : "unknown",
            target ? target : "-",
            status ? status : "-",
            detail ? detail : "-");
    fclose(fp);
  }
  pthread_mutex_unlock(&g_log_mutex);
}

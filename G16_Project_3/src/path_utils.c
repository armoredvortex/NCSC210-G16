#include "path_utils.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int is_path_safe(const char *path) {
  if (path == NULL || *path == '\0') {
    return 0;
  }
  if (path[0] == '/') {
    return 0;
  }
  if (strstr(path, "..") != NULL) {
    return 0;
  }
  return 1;
}

int path_build(const char *data_dir, const char *relative, char *out, size_t out_sz) {
  if (!is_path_safe(relative)) {
    return -1;
  }
  int n = snprintf(out, out_sz, "%s/%s", data_dir, relative);
  if (n < 0 || (size_t)n >= out_sz) {
    return -1;
  }
  return 0;
}

int path_ensure_parent_dirs(const char *filepath) {
  char tmp[PATH_MAX];
  int n = snprintf(tmp, sizeof(tmp), "%s", filepath);
  if (n < 0 || (size_t)n >= sizeof(tmp)) {
    return -1;
  }

  for (char *p = tmp + 1; *p != '\0'; ++p) {
    if (*p != '/') {
      continue;
    }
    *p = '\0';
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
      return -1;
    }
    *p = '/';
  }

  return 0;
}

int path_append_suffix(const char *path, const char *suffix, char *out, size_t out_sz) {
  int n = snprintf(out, out_sz, "%s%s", path, suffix);
  if (n < 0 || (size_t)n >= out_sz) {
    return -1;
  }
  return 0;
}

int path_strip_suffix(const char *path, const char *suffix, char *out, size_t out_sz) {
  size_t plen = strlen(path);
  size_t slen = strlen(suffix);
  if (plen <= slen || strcmp(path + plen - slen, suffix) != 0) {
    return -1;
  }
  size_t need = plen - slen;
  if (need + 1 > out_sz) {
    return -1;
  }
  memcpy(out, path, need);
  out[need] = '\0';
  return 0;
}

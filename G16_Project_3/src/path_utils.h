#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stddef.h>

int path_build(const char *data_dir, const char *relative, char *out, size_t out_sz);
int path_ensure_parent_dirs(const char *filepath);
int path_append_suffix(const char *path, const char *suffix, char *out, size_t out_sz);
int path_strip_suffix(const char *path, const char *suffix, char *out, size_t out_sz);

#endif

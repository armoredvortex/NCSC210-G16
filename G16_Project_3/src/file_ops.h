#ifndef FILE_OPS_H
#define FILE_OPS_H

#include <stddef.h>

int op_getfile(int fd, const char *data_dir, const char *client, const char *rel_path);
int op_setfile(int fd,
               const char *data_dir,
               const char *client,
               const char *rel_path,
               const char *data,
               size_t data_len);
int op_delfile(int fd, const char *data_dir, const char *client, const char *rel_path);
int op_renamefile(int fd,
                  const char *data_dir,
                  const char *client,
                  const char *src_rel,
                  const char *dst_rel);
int op_copyfile(int fd,
                const char *data_dir,
                const char *client,
                const char *src_rel,
                const char *dst_rel);
int op_statfile(int fd, const char *data_dir, const char *client, const char *rel_path);
int op_logs(int fd, const char *client, int line_limit);
int op_compress(int fd,
                const char *data_dir,
                const char *client,
                const char *src_rel,
                const char *dst_rel);
int op_decompress(int fd,
                  const char *data_dir,
                  const char *client,
                  const char *src_rel,
                  const char *dst_rel);

#endif

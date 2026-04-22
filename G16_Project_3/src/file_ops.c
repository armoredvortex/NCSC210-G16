#define _POSIX_C_SOURCE 200809L

#include "file_ops.h"

#include "locks.h"
#include "logger.h"
#include "path_utils.h"
#include "resp.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_FILE_SIZE (64 * 1024 * 1024)
#define COPY_BUF_SIZE 8192

static int read_all_file(const char *full_path, char **out_buf, size_t *out_len) {
  int f = open(full_path, O_RDONLY);
  if (f < 0) {
    return -1;
  }

  struct stat st;
  if (fstat(f, &st) != 0) {
    close(f);
    return -1;
  }

  if (st.st_size < 0 || st.st_size > (off_t)MAX_FILE_SIZE) {
    close(f);
    errno = EFBIG;
    return -1;
  }

  size_t sz = (size_t)st.st_size;
  char *buf = malloc(sz > 0 ? sz : 1);
  if (buf == NULL) {
    close(f);
    errno = ENOMEM;
    return -1;
  }

  size_t off = 0;
  while (off < sz) {
    ssize_t r = read(f, buf + off, sz - off);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      free(buf);
      close(f);
      return -1;
    }
    if (r == 0) {
      break;
    }
    off += (size_t)r;
  }

  close(f);
  *out_buf = buf;
  *out_len = off;
  return 0;
}

static int write_all_file(const char *full_path, const char *data, size_t len) {
  if (path_ensure_parent_dirs(full_path) != 0) {
    return -1;
  }

  int f = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (f < 0) {
    return -1;
  }

  size_t off = 0;
  while (off < len) {
    ssize_t w = write(f, data + off, len - off);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      close(f);
      return -1;
    }
    off += (size_t)w;
  }

  if (fsync(f) != 0) {
    close(f);
    return -1;
  }

  if (close(f) != 0) {
    return -1;
  }
  return 0;
}

int op_getfile(int fd, const char *data_dir, const char *client, const char *rel_path) {
  char full[PATH_MAX];
  if (path_build(data_dir, rel_path, full, sizeof(full)) != 0) {
    logger_log(client, "GETFILE", rel_path, "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *lock = NULL;
  if (locks_acquire_read(full, &lock) != 0) {
    logger_log(client, "GETFILE", rel_path, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  char *buf = NULL;
  size_t len = 0;
  if (read_all_file(full, &buf, &len) != 0) {
    int e = errno;
    locks_release(lock);
    if (e == ENOENT) {
      logger_log(client, "GETFILE", rel_path, "MISS", "not found");
      return resp_reply_null_bulk(fd);
    }
    if (e == EFBIG) {
      logger_log(client, "GETFILE", rel_path, "ERR", "file too large");
      return resp_reply_error(fd, "file too large");
    }
    logger_log(client, "GETFILE", rel_path, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  locks_release(lock);
  int rc = resp_reply_bulk(fd, buf, len);
  free(buf);
  logger_log(client, "GETFILE", rel_path, "OK", "read success");
  return rc;
}

int op_setfile(int fd,
               const char *data_dir,
               const char *client,
               const char *rel_path,
               const char *data,
               size_t data_len) {
  char full[PATH_MAX];
  if (path_build(data_dir, rel_path, full, sizeof(full)) != 0) {
    logger_log(client, "SETFILE", rel_path, "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *lock = NULL;
  if (locks_acquire_write(full, &lock) != 0) {
    logger_log(client, "SETFILE", rel_path, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  if (write_all_file(full, data, data_len) != 0) {
    int e = errno;
    locks_release(lock);
    logger_log(client, "SETFILE", rel_path, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  locks_release(lock);
  logger_log(client, "SETFILE", rel_path, "OK", "write success");
  return resp_reply_simple(fd, "OK");
}

int op_delfile(int fd, const char *data_dir, const char *client, const char *rel_path) {
  char full[PATH_MAX];
  if (path_build(data_dir, rel_path, full, sizeof(full)) != 0) {
    logger_log(client, "DELFILE", rel_path, "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *lock = NULL;
  if (locks_acquire_write(full, &lock) != 0) {
    logger_log(client, "DELFILE", rel_path, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  if (unlink(full) != 0) {
    int e = errno;
    locks_release(lock);
    if (e == ENOENT) {
      logger_log(client, "DELFILE", rel_path, "MISS", "not found");
      return resp_reply_integer(fd, 0);
    }
    logger_log(client, "DELFILE", rel_path, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  locks_release(lock);
  logger_log(client, "DELFILE", rel_path, "OK", "deleted");
  return resp_reply_integer(fd, 1);
}

int op_renamefile(int fd,
                  const char *data_dir,
                  const char *client,
                  const char *src_rel,
                  const char *dst_rel) {
  char src_full[PATH_MAX];
  char dst_full[PATH_MAX];

  if (path_build(data_dir, src_rel, src_full, sizeof(src_full)) != 0 ||
      path_build(data_dir, dst_rel, dst_full, sizeof(dst_full)) != 0) {
    logger_log(client, "RENAMEFILE", "-", "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *src_lock = NULL;
  pthread_rwlock_t *dst_lock = NULL;
  int same_lock = 0;
  if (locks_acquire_two_write(src_full, dst_full, &src_lock, &dst_lock, &same_lock) != 0) {
    logger_log(client, "RENAMEFILE", src_rel, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  if (path_ensure_parent_dirs(dst_full) != 0) {
    int e = errno;
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "RENAMEFILE", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  if (rename(src_full, dst_full) != 0) {
    int e = errno;
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "RENAMEFILE", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  locks_release_two(src_lock, dst_lock, same_lock);
  logger_log(client, "RENAMEFILE", src_rel, "OK", "renamed");
  return resp_reply_simple(fd, "OK");
}

int op_copyfile(int fd,
                const char *data_dir,
                const char *client,
                const char *src_rel,
                const char *dst_rel) {
  char src_full[PATH_MAX];
  char dst_full[PATH_MAX];

  if (path_build(data_dir, src_rel, src_full, sizeof(src_full)) != 0 ||
      path_build(data_dir, dst_rel, dst_full, sizeof(dst_full)) != 0) {
    logger_log(client, "COPYFILE", "-", "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *src_lock = NULL;
  pthread_rwlock_t *dst_lock = NULL;
  int same_lock = 0;
  if (locks_acquire_two_write(src_full, dst_full, &src_lock, &dst_lock, &same_lock) != 0) {
    logger_log(client, "COPYFILE", src_rel, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  int src_fd = open(src_full, O_RDONLY);
  if (src_fd < 0) {
    int e = errno;
    locks_release_two(src_lock, dst_lock, same_lock);
    if (e == ENOENT) {
      logger_log(client, "COPYFILE", src_rel, "MISS", "source not found");
      return resp_reply_error(fd, "source file not found");
    }
    logger_log(client, "COPYFILE", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  if (path_ensure_parent_dirs(dst_full) != 0) {
    int e = errno;
    close(src_fd);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "COPYFILE", dst_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  int dst_fd = open(dst_full, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dst_fd < 0) {
    int e = errno;
    close(src_fd);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "COPYFILE", dst_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  char buf[COPY_BUF_SIZE];
  for (;;) {
    ssize_t r = read(src_fd, buf, sizeof(buf));
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      int e = errno;
      close(src_fd);
      close(dst_fd);
      locks_release_two(src_lock, dst_lock, same_lock);
      logger_log(client, "COPYFILE", src_rel, "ERR", strerror(e));
      return resp_reply_error(fd, strerror(e));
    }
    if (r == 0) {
      break;
    }

    size_t off = 0;
    while (off < (size_t)r) {
      ssize_t w = write(dst_fd, buf + off, (size_t)r - off);
      if (w < 0) {
        if (errno == EINTR) {
          continue;
        }
        int e = errno;
        close(src_fd);
        close(dst_fd);
        locks_release_two(src_lock, dst_lock, same_lock);
        logger_log(client, "COPYFILE", dst_rel, "ERR", strerror(e));
        return resp_reply_error(fd, strerror(e));
      }
      off += (size_t)w;
    }
  }

  if (fsync(dst_fd) != 0) {
    int e = errno;
    close(src_fd);
    close(dst_fd);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "COPYFILE", dst_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  close(src_fd);
  close(dst_fd);
  locks_release_two(src_lock, dst_lock, same_lock);

  logger_log(client, "COPYFILE", src_rel, "OK", "copied");
  return resp_reply_simple(fd, "OK");
}

int op_statfile(int fd, const char *data_dir, const char *client, const char *rel_path) {
  char full[PATH_MAX];
  if (path_build(data_dir, rel_path, full, sizeof(full)) != 0) {
    logger_log(client, "STATFILE", rel_path, "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *lock = NULL;
  if (locks_acquire_read(full, &lock) != 0) {
    logger_log(client, "STATFILE", rel_path, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  struct stat st;
  if (stat(full, &st) != 0) {
    int e = errno;
    locks_release(lock);
    if (e == ENOENT) {
      logger_log(client, "STATFILE", rel_path, "MISS", "not found");
      return resp_reply_null_bulk(fd);
    }
    logger_log(client, "STATFILE", rel_path, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  locks_release(lock);

  char out[512];
  int n = snprintf(out,
                   sizeof(out),
                   "size=%lld mode=%o uid=%u gid=%u atime=%lld mtime=%lld ctime=%lld",
                   (long long)st.st_size,
                   st.st_mode & 0777,
                   st.st_uid,
                   st.st_gid,
                   (long long)st.st_atime,
                   (long long)st.st_mtime,
                   (long long)st.st_ctime);
  if (n < 0 || (size_t)n >= sizeof(out)) {
    logger_log(client, "STATFILE", rel_path, "ERR", "stat formatting error");
    return resp_reply_error(fd, "stat formatting error");
  }

  logger_log(client, "STATFILE", rel_path, "OK", "metadata read");
  return resp_reply_bulk(fd, out, (size_t)n);
}

static int read_log_tail(int line_limit, char **out, size_t *out_len) {
  FILE *fp = fopen(logger_path(), "r");
  if (fp == NULL) {
    if (errno == ENOENT) {
      *out = strdup("");
      *out_len = 0;
      return *out ? 0 : -1;
    }
    return -1;
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return -1;
  }

  long size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    return -1;
  }

  if (size > 1024 * 1024) {
    size = 1024 * 1024;
  }

  if (fseek(fp, -size, SEEK_END) != 0) {
    if (fseek(fp, 0, SEEK_SET) != 0) {
      fclose(fp);
      return -1;
    }
  }

  char *buf = malloc((size_t)size + 1);
  if (buf == NULL) {
    fclose(fp);
    return -1;
  }

  size_t nread = fread(buf, 1, (size_t)size, fp);
  fclose(fp);
  buf[nread] = '\0';

  if (line_limit <= 0) {
    *out = buf;
    *out_len = nread;
    return 0;
  }

  int seen = 0;
  size_t start = 0;
  for (size_t i = nread; i > 0; --i) {
    if (buf[i - 1] == '\n') {
      seen++;
      if (seen > line_limit) {
        start = i;
        break;
      }
    }
  }

  size_t out_sz = nread - start;
  char *trim = malloc(out_sz + 1);
  if (trim == NULL) {
    free(buf);
    return -1;
  }
  memcpy(trim, buf + start, out_sz);
  trim[out_sz] = '\0';
  free(buf);

  *out = trim;
  *out_len = out_sz;
  return 0;
}

int op_logs(int fd, const char *client, int line_limit) {
  char *buf = NULL;
  size_t len = 0;
  if (read_log_tail(line_limit, &buf, &len) != 0) {
    logger_log(client, "LOGS", "operations.log", "ERR", strerror(errno));
    return resp_reply_error(fd, strerror(errno));
  }

  int rc = resp_reply_bulk(fd, buf, len);
  free(buf);
  logger_log(client, "LOGS", "operations.log", "OK", "read logs");
  return rc;
}

static int rle_compress_bytes(const unsigned char *in,
                              size_t in_len,
                              unsigned char **out,
                              size_t *out_len) {
  size_t cap = in_len * 2 + 8;
  unsigned char *buf = malloc(cap == 0 ? 8 : cap);
  if (buf == NULL) {
    errno = ENOMEM;
    return -1;
  }

  size_t w = 0;
  buf[w++] = 'R';
  buf[w++] = 'L';
  buf[w++] = 'E';
  buf[w++] = '1';

  for (size_t i = 0; i < in_len;) {
    unsigned char value = in[i];
    unsigned int run = 1;
    while (i + run < in_len && in[i + run] == value && run < 255) {
      run++;
    }
    buf[w++] = (unsigned char)run;
    buf[w++] = value;
    i += run;
  }

  *out = buf;
  *out_len = w;
  return 0;
}

static int rle_decompress_bytes(const unsigned char *in,
                                size_t in_len,
                                unsigned char **out,
                                size_t *out_len) {
  if (in_len < 4 || in[0] != 'R' || in[1] != 'L' || in[2] != 'E' || in[3] != '1') {
    errno = EINVAL;
    return -1;
  }

  size_t total = 0;
  for (size_t i = 4; i + 1 < in_len; i += 2) {
    total += in[i];
    if (total > MAX_FILE_SIZE) {
      errno = EFBIG;
      return -1;
    }
  }

  if (((in_len - 4) % 2) != 0) {
    errno = EINVAL;
    return -1;
  }

  unsigned char *buf = malloc(total > 0 ? total : 1);
  if (buf == NULL) {
    errno = ENOMEM;
    return -1;
  }

  size_t w = 0;
  for (size_t i = 4; i + 1 < in_len; i += 2) {
    unsigned char run = in[i];
    unsigned char value = in[i + 1];
    memset(buf + w, value, run);
    w += run;
  }

  *out = buf;
  *out_len = w;
  return 0;
}

int op_compress(int fd,
                const char *data_dir,
                const char *client,
                const char *src_rel,
                const char *dst_rel) {
  char src_full[PATH_MAX];
  char dst_full[PATH_MAX];

  if (path_build(data_dir, src_rel, src_full, sizeof(src_full)) != 0 ||
      path_build(data_dir, dst_rel, dst_full, sizeof(dst_full)) != 0) {
    logger_log(client, "COMPRESS", src_rel, "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *src_lock = NULL;
  pthread_rwlock_t *dst_lock = NULL;
  int same_lock = 0;
  if (locks_acquire_two_write(src_full, dst_full, &src_lock, &dst_lock, &same_lock) != 0) {
    logger_log(client, "COMPRESS", src_rel, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  char *src_buf = NULL;
  size_t src_len = 0;
  if (read_all_file(src_full, &src_buf, &src_len) != 0) {
    int e = errno;
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "COMPRESS", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  unsigned char *comp = NULL;
  size_t comp_len = 0;
  if (rle_compress_bytes((unsigned char *)src_buf, src_len, &comp, &comp_len) != 0) {
    int e = errno;
    free(src_buf);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "COMPRESS", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  if (write_all_file(dst_full, (char *)comp, comp_len) != 0) {
    int e = errno;
    free(src_buf);
    free(comp);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "COMPRESS", dst_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  free(src_buf);
  free(comp);
  locks_release_two(src_lock, dst_lock, same_lock);
  logger_log(client, "COMPRESS", src_rel, "OK", "compressed");
  return resp_reply_simple(fd, "OK");
}

int op_decompress(int fd,
                  const char *data_dir,
                  const char *client,
                  const char *src_rel,
                  const char *dst_rel) {
  char src_full[PATH_MAX];
  char dst_full[PATH_MAX];

  if (path_build(data_dir, src_rel, src_full, sizeof(src_full)) != 0 ||
      path_build(data_dir, dst_rel, dst_full, sizeof(dst_full)) != 0) {
    logger_log(client, "DECOMPRESS", src_rel, "ERR", "invalid path");
    return resp_reply_error(fd, "invalid path");
  }

  pthread_rwlock_t *src_lock = NULL;
  pthread_rwlock_t *dst_lock = NULL;
  int same_lock = 0;
  if (locks_acquire_two_write(src_full, dst_full, &src_lock, &dst_lock, &same_lock) != 0) {
    logger_log(client, "DECOMPRESS", src_rel, "ERR", "lock failure");
    return resp_reply_error(fd, "internal lock error");
  }

  char *src_buf = NULL;
  size_t src_len = 0;
  if (read_all_file(src_full, &src_buf, &src_len) != 0) {
    int e = errno;
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "DECOMPRESS", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  unsigned char *dec = NULL;
  size_t dec_len = 0;
  if (rle_decompress_bytes((unsigned char *)src_buf, src_len, &dec, &dec_len) != 0) {
    int e = errno;
    free(src_buf);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "DECOMPRESS", src_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  if (write_all_file(dst_full, (char *)dec, dec_len) != 0) {
    int e = errno;
    free(src_buf);
    free(dec);
    locks_release_two(src_lock, dst_lock, same_lock);
    logger_log(client, "DECOMPRESS", dst_rel, "ERR", strerror(e));
    return resp_reply_error(fd, strerror(e));
  }

  free(src_buf);
  free(dec);
  locks_release_two(src_lock, dst_lock, same_lock);
  logger_log(client, "DECOMPRESS", src_rel, "OK", "decompressed");
  return resp_reply_simple(fd, "OK");
}

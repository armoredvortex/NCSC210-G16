#include "locks.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct file_lock_entry {
  char *path;
  pthread_rwlock_t rwlock;
  struct file_lock_entry *next;
} file_lock_entry_t;

static pthread_mutex_t g_lock_map_mutex = PTHREAD_MUTEX_INITIALIZER;
static file_lock_entry_t *g_lock_map_head = NULL;

static file_lock_entry_t *get_or_create_lock(const char *canonical_path) {
  pthread_mutex_lock(&g_lock_map_mutex);

  file_lock_entry_t *cur = g_lock_map_head;
  while (cur != NULL) {
    if (strcmp(cur->path, canonical_path) == 0) {
      pthread_mutex_unlock(&g_lock_map_mutex);
      return cur;
    }
    cur = cur->next;
  }

  file_lock_entry_t *node = calloc(1, sizeof(*node));
  if (node == NULL) {
    pthread_mutex_unlock(&g_lock_map_mutex);
    return NULL;
  }

  node->path = strdup(canonical_path);
  if (node->path == NULL) {
    free(node);
    pthread_mutex_unlock(&g_lock_map_mutex);
    return NULL;
  }

  if (pthread_rwlock_init(&node->rwlock, NULL) != 0) {
    free(node->path);
    free(node);
    pthread_mutex_unlock(&g_lock_map_mutex);
    return NULL;
  }

  node->next = g_lock_map_head;
  g_lock_map_head = node;

  pthread_mutex_unlock(&g_lock_map_mutex);
  return node;
}

int locks_acquire_read(const char *canonical_path, pthread_rwlock_t **held) {
  file_lock_entry_t *entry = get_or_create_lock(canonical_path);
  if (entry == NULL) {
    return -1;
  }
  if (pthread_rwlock_rdlock(&entry->rwlock) != 0) {
    return -1;
  }
  *held = &entry->rwlock;
  return 0;
}

int locks_acquire_write(const char *canonical_path, pthread_rwlock_t **held) {
  file_lock_entry_t *entry = get_or_create_lock(canonical_path);
  if (entry == NULL) {
    return -1;
  }
  if (pthread_rwlock_wrlock(&entry->rwlock) != 0) {
    return -1;
  }
  *held = &entry->rwlock;
  return 0;
}

int locks_acquire_two_write(const char *path_a,
                            const char *path_b,
                            pthread_rwlock_t **lock_a,
                            pthread_rwlock_t **lock_b,
                            int *same_lock) {
  *lock_a = NULL;
  *lock_b = NULL;
  *same_lock = 0;

  if (strcmp(path_a, path_b) == 0) {
    if (locks_acquire_write(path_a, lock_a) != 0) {
      return -1;
    }
    *lock_b = *lock_a;
    *same_lock = 1;
    return 0;
  }

  const char *first = path_a;
  const char *second = path_b;
  int swapped = 0;

  if (strcmp(path_a, path_b) > 0) {
    first = path_b;
    second = path_a;
    swapped = 1;
  }

  pthread_rwlock_t *first_lock = NULL;
  pthread_rwlock_t *second_lock = NULL;

  if (locks_acquire_write(first, &first_lock) != 0) {
    return -1;
  }
  if (locks_acquire_write(second, &second_lock) != 0) {
    pthread_rwlock_unlock(first_lock);
    return -1;
  }

  if (!swapped) {
    *lock_a = first_lock;
    *lock_b = second_lock;
  } else {
    *lock_a = second_lock;
    *lock_b = first_lock;
  }

  return 0;
}

void locks_release(pthread_rwlock_t *lock) {
  if (lock != NULL) {
    pthread_rwlock_unlock(lock);
  }
}

void locks_release_two(pthread_rwlock_t *lock_a, pthread_rwlock_t *lock_b, int same_lock) {
  if (same_lock) {
    if (lock_a != NULL) {
      pthread_rwlock_unlock(lock_a);
    }
    return;
  }
  if (lock_a != NULL) {
    pthread_rwlock_unlock(lock_a);
  }
  if (lock_b != NULL) {
    pthread_rwlock_unlock(lock_b);
  }
}

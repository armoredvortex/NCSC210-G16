#ifndef LOCKS_H
#define LOCKS_H

#include <pthread.h>

int locks_acquire_read(const char *canonical_path, pthread_rwlock_t **held);
int locks_acquire_write(const char *canonical_path, pthread_rwlock_t **held);
int locks_acquire_two_write(const char *path_a,
                            const char *path_b,
                            pthread_rwlock_t **lock_a,
                            pthread_rwlock_t **lock_b,
                            int *same_lock);
void locks_release(pthread_rwlock_t *lock);
void locks_release_two(pthread_rwlock_t *lock_a, pthread_rwlock_t *lock_b, int same_lock);

#endif

#ifndef SHM_H
#define SHM_H

#define MAX_SHM       8        // max shared memory segments
#define MAX_SHM_PG    4        // max pages per segment
#define MAX_SHM_SIZE  (MAX_SHM_PG * 4096)  // 16KB per segment

struct shmseg {
  int used;              // slot in use?
  int key;               // user-provided key
  int size;              // requested size (bytes)
  int npages;            // number of physical pages allocated
  uint64 pa[MAX_SHM_PG]; // physical addresses of allocated pages
  int refcount;          // number of processes currently attached
};

#endif

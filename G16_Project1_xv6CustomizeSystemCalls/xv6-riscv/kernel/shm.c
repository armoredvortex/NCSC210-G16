#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "shm.h"

struct shmseg shmtable[MAX_SHM];
struct spinlock shmlock;

void
shminit(void)
{
  initlock(&shmlock, "shmlock");
}

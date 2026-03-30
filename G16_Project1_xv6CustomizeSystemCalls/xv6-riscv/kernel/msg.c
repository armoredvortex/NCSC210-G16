#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "msg.h"

struct msgqueue msgq[MAX_Q]; // array of queues
struct spinlock msgqlock; // spinlock for queues

// initialize queues (called in kernel/main.c)
void msginit() {
    initlock(&msgqlock, "msgqlock");
    for (int i = 0; i < MAX_Q; i++) {
        msgq[i].used  = 0;
        msgq[i].front = 0;
        msgq[i].rear  = 0;
        msgq[i].count = 0;
    }
}

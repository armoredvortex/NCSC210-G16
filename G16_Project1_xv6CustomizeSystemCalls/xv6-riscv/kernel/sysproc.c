#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "msg.h"  // for message queues 

#define NKMUTEX 16

struct kmutex_entry {
  int used;
  int locked;
  int owner_pid;
};

extern struct msgqueue msgq[MAX_Q]; // array of queues (defined in msg.c)
extern struct spinlock msgqlock; 
extern struct spinlock wait_lock;
static struct kmutex_entry kmutex_table[NKMUTEX];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_getppid(void)
{
  int ppid = 0;
  struct proc *p = myproc();

  acquire(&wait_lock);
  if(p->parent)
    ppid = p->parent->pid;
  release(&wait_lock);

  return ppid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_forkn(void)
{
  int n;

  argint(0, &n);
  if(n <= 0)
    return -1;
  return kforkn(n);
}

uint64
sys_thread_create(void)
{
  uint64 fn;
  uint64 arg;

  argaddr(0, &fn);
  argaddr(1, &arg);

  return kthread_create(fn, arg);
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_waitpid(void)
{
  int pid;
  uint64 status_addr;

  argint(0, &pid);
  argaddr(1, &status_addr);
  return kwaitpid(pid, status_addr);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_mutex_create(void)
{
  int i;

  acquire(&wait_lock);
  for(i = 0; i < NKMUTEX; i++) {
    if(kmutex_table[i].used == 0) {
      kmutex_table[i].used = 1;
      kmutex_table[i].locked = 0;
      kmutex_table[i].owner_pid = -1;
      release(&wait_lock);
      return i;
    }
  }
  release(&wait_lock);
  return -1;
}

uint64
sys_mutex_lock(void)
{
  int id;
  struct proc *p;

  argint(0, &id);
  if(id < 0 || id >= NKMUTEX)
    return -1;

  p = myproc();
  acquire(&wait_lock);
  while(1) {
    if(kmutex_table[id].used == 0) {
      release(&wait_lock);
      return -1;
    }
    if(kmutex_table[id].locked == 0) {
      kmutex_table[id].locked = 1;
      kmutex_table[id].owner_pid = p->pid;
      release(&wait_lock);
      return 0;
    }
    if(kmutex_table[id].owner_pid == p->pid) {
      // Non-recursive mutex: same owner cannot lock twice.
      release(&wait_lock);
      return -1;
    }
    sleep(&kmutex_table[id], &wait_lock);
    if(killed(p)) {
      release(&wait_lock);
      return -1;
    }
  }
}

uint64
sys_mutex_unlock(void)
{
  int id;
  struct proc *p;

  argint(0, &id);
  if(id < 0 || id >= NKMUTEX)
    return -1;

  p = myproc();
  acquire(&wait_lock);
  if(kmutex_table[id].used == 0 || kmutex_table[id].locked == 0) {
    release(&wait_lock);
    return -1;
  }
  if(kmutex_table[id].owner_pid != p->pid) {
    release(&wait_lock);
    return -1;
  }

  kmutex_table[id].locked = 0;
  kmutex_table[id].owner_pid = -1;
  wakeup(&kmutex_table[id]);
  release(&wait_lock);
  return 0;
}

uint64
sys_mutex_trylock(void)
{
  int id;
  struct proc *p;

  argint(0, &id);
  if(id < 0 || id >= NKMUTEX)
    return -1;

  p = myproc();
  acquire(&wait_lock);
  if(kmutex_table[id].used == 0) {
    release(&wait_lock);
    return -1;
  }
  if(kmutex_table[id].locked == 0) {
    kmutex_table[id].locked = 1;
    kmutex_table[id].owner_pid = p->pid;
    release(&wait_lock);
    return 0;
  }
  release(&wait_lock);
  return -1;
}

uint64
sys_mutex_owner(void)
{
  int id;

  argint(0, &id);
  if(id < 0 || id >= NKMUTEX)
    return -1;

  acquire(&wait_lock);
  if(kmutex_table[id].used == 0){
    release(&wait_lock);
    return -1;
  }

  int owner = kmutex_table[id].owner_pid;
  if(kmutex_table[id].locked == 0)
    owner = 0;
  release(&wait_lock);
  return owner;
}

uint64
sys_freeze(void)
{
  int pid;

  argint(0, &pid);
  return kfreeze(pid);
}

uint64
sys_thaw(void)
{
  int pid;

  argint(0, &pid);
  return kthaw(pid);
}

uint64
sys_signal_send(void)
{
  int pid;
  int sig;

  argint(0, &pid);
  argint(1, &sig);

  return ksignal_send(pid, sig);
}

uint64
sys_getprocstate(void)
{
  int pid;

  argint(0, &pid);
  if(pid <= 0)
    return -1;

  return kgetprocstate(pid);
}

uint64  
sys_msgget(void){

  //get the key from user buffer
  int key; 
  argint(0, &key); 

  //acquire lock 
  acquire(&msgqlock); 

  //1. check if msgque with key exists 
  for(int i=0; i<MAX_Q; i++){
    if(msgq[i].used && msgq[i].key==key){

      //release lock 
      release(&msgqlock);
      return i;
    }
  }

  //2. create a new msgque with key
  for(int i=0; i<MAX_Q; i++){
    if(!msgq[i].used){
      msgq[i].used = 1; 
      msgq[i].key = key;
      msgq[i].count = 0;

      //release lock 
      release(&msgqlock);
      return i;
    }
  }

  //release lock 
  release(&msgqlock); 
  
  //3. no free space found in msgque 
  return -1;
}

uint64 
sys_sendmsg(void){

  int quid, type, len;
  uint64 msgptr;
  struct msgqueue *q;

  //get the arguments from user buffer
  argint(0, &quid);
  argint(1, &type); 
  argaddr(2, &msgptr); 
  argint(3, &len);

    //acquire lock 
  acquire(&msgqlock);

  //validate queue id 
  if(quid < 0 || quid >= MAX_Q || !msgq[quid].used){
    release(&msgqlock);
    return -1; 
  }

  //validate msg len
  if(len <= 0 || len > MAX_MSG_LEN){
    release(&msgqlock);
    return -1;
  }

  q = &msgq[quid]; 

  //check if queue is full 
  if(q->count == MAX_MSG){
    //queue is full can't send message
    release(&msgqlock);
    return -1;
  }

  int idx = -1; 
  for(int i=0; i<MAX_MSG; i++){
    if(!q->msg[i].valid){
      idx = i; break;
    }
  }

  if(idx == -1){
    release(&msgqlock);
    return -1;
  }

  struct message *m = &q->msg[idx];
  m->type = type; 
  m->len = len;
  
  //copy data from user buffer to kernel buffer 
  if(copyin(myproc()->pagetable, m->data, msgptr, len) < 0){
    release(&msgqlock);
    return -1;
  }

   m->valid = 1;  //makr message valid only after copyin is successful
   q->count++;   //no.of messages in queue

  //release lock
  release(&msgqlock);
  return 0;
}

uint64 
sys_recvmsg(void){
  int quid, type;
  int bufflen; //length of user buffer (in bytes)
  uint64 msgptr; //user buffer 
  struct msgqueue *q; 
  struct message *m; 

  //get the arguments from user buffer 
  argint(0, &quid);
  argint(1, &type); 
  argaddr(2, &msgptr);
  argint(3, &bufflen);

  //acquire lock 
  acquire(&msgqlock); 

  //validate queue id
  if(quid < 0 || quid >= MAX_Q || !msgq[quid].used){
    release(&msgqlock);
    return -1; 
  }

  q = &msgq[quid];  //get the queue

  //check for the required message type
  int idx = -1; 
  for(int i=0; i<MAX_MSG; i++){
    if(q->msg[i].valid && q->msg[i].type == type){
      idx = i; break;
    }
  }
  
  if(idx == -1){
    //no message found with the required type
    //release lock 
    release(&msgqlock);
    return -1;
  }

  m = &q->msg[idx]; 
  int copylen = m->len < bufflen ? m->len : bufflen; //length to be copied to user buffer

  //copy data from kernel buffer(m->data) to user buffer(msgptr)
  if(copyout(myproc()->pagetable, msgptr, m->data, copylen) < 0){
    release(&msgqlock);
    return -1; 
  }

  m->valid = 0; //mark message as invalid
  q->count--; //decrement no.of messages in queue


  //release lock 
  release(&msgqlock);
  return copylen;
}

uint64
sys_msgcount(void)
{
  int quid;

  argint(0, &quid);

  acquire(&msgqlock);
  if(quid < 0 || quid >= MAX_Q || !msgq[quid].used){
    release(&msgqlock);
    return -1;
  }

  int count = msgq[quid].count;
  release(&msgqlock);
  return count;
}

// ---- Shared Memory System Calls ----

#include "shm.h"

extern struct shmseg shmtable[MAX_SHM];
extern struct spinlock shmlock;

// shmget(key, size) - create or get a shared memory segment
uint64
sys_shmget(void)
{
  int key, size;
  argint(0, &key);
  argint(1, &size);

  if(size <= 0 || size > MAX_SHM_SIZE)
    return -1;

  int npages = (size + PGSIZE - 1) / PGSIZE;

  acquire(&shmlock);

  // 1. Check if segment with key already exists
  for(int i = 0; i < MAX_SHM; i++){
    if(shmtable[i].used && shmtable[i].key == key){
      release(&shmlock);
      return i;
    }
  }

  // 2. Allocate a new segment
  for(int i = 0; i < MAX_SHM; i++){
    if(!shmtable[i].used){
      shmtable[i].used = 1;
      shmtable[i].key = key;
      shmtable[i].size = size;
      shmtable[i].npages = npages;
      shmtable[i].refcount = 0;

      // Allocate physical pages
      int j;
      for(j = 0; j < npages; j++){
        void *mem = kalloc();
        if(mem == 0){
          // Free already allocated pages on failure
          for(int k = 0; k < j; k++)
            kfree((void*)shmtable[i].pa[k]);
          shmtable[i].used = 0;
          release(&shmlock);
          return -1;
        }
        memset(mem, 0, PGSIZE);
        shmtable[i].pa[j] = (uint64)mem;
      }

      release(&shmlock);
      return i;
    }
  }

  release(&shmlock);
  return -1;  // no free slot
}

// shmat(id) - attach shared memory segment to process address space
uint64
sys_shmat(void)
{
  int id;
  argint(0, &id);

  if(id < 0 || id >= MAX_SHM)
    return -1;

  struct proc *p = myproc();

  acquire(&shmlock);

  if(!shmtable[id].used){
    release(&shmlock);
    return -1;
  }

  // Check if already attached
  if(p->shm_va[id] != 0){
    release(&shmlock);
    return p->shm_va[id];  // return existing mapping
  }

  // Map at a fixed virtual address below TRAPFRAME
  // Each segment gets its own region: TRAPFRAME - (id+1)*MAX_SHM_SIZE
  uint64 va = TRAPFRAME - (uint64)(id + 1) * MAX_SHM_SIZE;

  for(int j = 0; j < shmtable[id].npages; j++){
    if(mappages(p->pagetable, va + j * PGSIZE, PGSIZE,
                shmtable[id].pa[j], PTE_R | PTE_W | PTE_U) != 0){
      // Undo mappings on failure
      for(int k = 0; k < j; k++){
        uvmunmap(p->pagetable, va + k * PGSIZE, 1, 0);
      }
      release(&shmlock);
      return -1;
    }
  }

  p->shm_va[id] = va;
  shmtable[id].refcount++;

  release(&shmlock);
  return va;
}

// shmdt(id) - detach shared memory segment from process address space
uint64
sys_shmdt(void)
{
  int id;
  argint(0, &id);

  if(id < 0 || id >= MAX_SHM)
    return -1;

  struct proc *p = myproc();

  acquire(&shmlock);

  if(!shmtable[id].used || p->shm_va[id] == 0){
    release(&shmlock);
    return -1;
  }

  uint64 va = p->shm_va[id];

  // Unmap pages (do_free=0 because physical pages are shared)
  uvmunmap(p->pagetable, va, shmtable[id].npages, 0);
  p->shm_va[id] = 0;

  shmtable[id].refcount--;

  // If no one is attached anymore, free the physical pages and the slot
  if(shmtable[id].refcount <= 0){
    for(int j = 0; j < shmtable[id].npages; j++){
      kfree((void*)shmtable[id].pa[j]);
      shmtable[id].pa[j] = 0;
    }
    shmtable[id].used = 0;
    shmtable[id].key = 0;
    shmtable[id].size = 0;
    shmtable[id].npages = 0;
    shmtable[id].refcount = 0;
  }

  release(&shmlock);
  return 0;
}
#include "kernel/types.h"
#include "user/user.h"

static const char*
state_name(int st)
{
  if(st == 0) return "UNUSED";
  if(st == 1) return "USED";
  if(st == 2) return "SLEEPING";
  if(st == 3) return "RUNNABLE";
  if(st == 4) return "RUNNING";
  if(st == 5) return "ZOMBIE";
  return "UNKNOWN";
}

static void
thread_entry(void *arg)
{
  int id = (int)(uint64)arg;
  printf("thread[%d] pid=%d ppid=%d\n", id, getpid(), getppid());
  pause(4);
  exit(40 + id);
}

int
main(void)
{
  int st = -1;
  int self = getpid();

  printf("sys2demo: begin pid=%d\n", self);

  // New syscall #1: getprocstate(pid)
  st = getprocstate(self);
  printf("getprocstate(self)=%d (%s)\n", st, state_name(st));

  // Lock functionality + new syscall #2: mutex_owner(mid)
  int mid = mutex_create();
  if(mid < 0){
    printf("mutex_create failed\n");
    exit(1);
  }
  printf("mutex_owner before lock=%d\n", mutex_owner(mid));
  if(mutex_lock(mid) < 0){
    printf("mutex_lock failed\n");
    exit(1);
  }
  printf("mutex_owner while locked=%d\n", mutex_owner(mid));
  mutex_unlock(mid);
  printf("mutex_owner after unlock=%d\n", mutex_owner(mid));

  // Process creation functionality.
  int c = fork();
  if(c < 0){
    printf("fork failed\n");
    exit(1);
  }
  if(c == 0){
    pause(30);
    exit(0);
  }

  // Signal functionality.
  printf("signal_send STOP to child=%d\n", c);
  signal_send(c, SIG_STOP);
  pause(10);
  printf("child state after STOP=%d (%s)\n", getprocstate(c), state_name(getprocstate(c)));

  printf("signal_send CONT to child=%d\n", c);
  signal_send(c, SIG_CONT);
  pause(10);

  // IPC functionality.
  int qid = msgget(4242);
  if(qid < 0){
    printf("msgget failed\n");
    signal_send(c, SIG_TERM);
    wait(0);
    exit(1);
  }
  sendmsg(qid, 7, "ipc-ok", 7);
  printf("msgcount=%d\n", msgcount(qid));

  char b[16];
  int n = recvmsg(qid, 7, b, sizeof(b));
  printf("recvmsg len=%d text=%s\n", n, b);

  // Thread functionality.
  int t = thread_create((void*)thread_entry, (void*)1);
  if(t < 0){
    printf("thread_create failed\n");
    signal_send(c, SIG_TERM);
    wait(0);
    exit(1);
  }
  waitpid(t, &st);
  printf("joined thread pid=%d status=%d\n", t, st);

  signal_send(c, SIG_TERM);
  waitpid(c, &st);
  printf("joined child pid=%d status=%d\n", c, st);

  printf("sys2demo: complete\n");
  exit(0);
}

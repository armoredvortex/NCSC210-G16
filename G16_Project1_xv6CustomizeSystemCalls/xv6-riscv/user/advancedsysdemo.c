#include "kernel/types.h"
#include "user/user.h"

static void
thread_worker(void *arg)
{
  uint64 id = (uint64)arg;
  printf("thread_worker[%d]: pid=%d ppid=%d\n", (int)id, getpid(), getppid());
  pause(4);
  exit((int)(100 + id));
}

int
main(void)
{
  int status = -1;

  printf("advancedsysdemo: begin\n");

  // 1) Process creation extension: forkn(n)
  int created = forkn(2);
  if(created < 0){
    printf("forkn failed\n");
    exit(1);
  }
  if(created == 0){
    printf("forkn-child: pid=%d running\n", getpid());
    exit(0);
  }
  printf("forkn-parent: created=%d children\n", created);
  for(int i = 0; i < created; i++)
    wait(&status);

  // 2) Thread-style creation: thread_create(start,arg)
  int t1 = thread_create((void*)thread_worker, (void*)1);
  int t2 = thread_create((void*)thread_worker, (void*)2);
  if(t1 < 0 || t2 < 0){
    printf("thread_create failed\n");
    exit(1);
  }
  printf("thread_create: t1=%d t2=%d\n", t1, t2);
  waitpid(t1, &status);
  printf("joined thread pid=%d status=%d\n", t1, status);
  waitpid(t2, &status);
  printf("joined thread pid=%d status=%d\n", t2, status);

  // 3) Lock extension: mutex_trylock
  int mid = mutex_create();
  if(mid < 0){
    printf("mutex_create failed\n");
    exit(1);
  }
  if(mutex_lock(mid) < 0){
    printf("mutex_lock failed\n");
    exit(1);
  }
  if(mutex_trylock(mid) == 0)
    printf("mutex_trylock: unexpected success while already held\n");
  else
    printf("mutex_trylock: correctly failed while held\n");
  mutex_unlock(mid);
  if(mutex_trylock(mid) == 0){
    printf("mutex_trylock: success on unlocked mutex\n");
    mutex_unlock(mid);
  }

  // 4) IPC extension: msgcount
  int qid = msgget(9090);
  if(qid < 0){
    printf("msgget failed\n");
    exit(1);
  }
  if(sendmsg(qid, 1, "hello", 6) < 0){
    printf("sendmsg failed\n");
    exit(1);
  }
  printf("msgcount after send=%d\n", msgcount(qid));
  char buf[16];
  int n = recvmsg(qid, 1, buf, sizeof(buf));
  if(n < 0){
    printf("recvmsg failed\n");
    exit(1);
  }
  printf("recvmsg got=%s len=%d\n", buf, n);
  printf("msgcount after recv=%d\n", msgcount(qid));

  // 5) Signal extension: signal_send(pid,sig)
  int target = fork();
  if(target < 0){
    printf("fork for signal demo failed\n");
    exit(1);
  }
  if(target == 0){
    while(1)
      pause(5);
  }

  pause(4);
  printf("signal_send: STOP -> pid=%d\n", target);
  signal_send(target, SIG_STOP);
  pause(6);
  printf("signal_send: CONT -> pid=%d\n", target);
  signal_send(target, SIG_CONT);
  pause(6);
  printf("signal_send: TERM -> pid=%d\n", target);
  signal_send(target, SIG_TERM);
  waitpid(target, &status);

  printf("advancedsysdemo: complete\n");
  exit(0);
}

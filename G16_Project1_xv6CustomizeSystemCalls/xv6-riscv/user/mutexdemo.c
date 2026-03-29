#include "kernel/types.h"
#include "user/user.h"

static void
say(const char *msg)
{
  write(1, msg, strlen(msg));
}

int
main(void)
{
  int mid = mutex_create();
  int pid;

  if(mid < 0){
    say("mutexdemo: mutex_create failed\n");
    exit(1);
  }

  pid = fork();
  if(pid < 0){
    say("mutexdemo: fork failed\n");
    exit(1);
  }

  if(pid == 0){
    pause(5);
    say("child: trying to lock mutex\n");
    if(mutex_lock(mid) < 0){
      say("child: mutex_lock failed\n");
      exit(1);
    }
    say("child: acquired mutex\n");
    pause(10);
    mutex_unlock(mid);
    say("child: released mutex\n");
    exit(0);
  }

  if(mutex_lock(mid) < 0){
    say("parent: mutex_lock failed\n");
    exit(1);
  }
  say("parent: acquired mutex\n");
  pause(30);
  say("parent: releasing mutex\n");
  mutex_unlock(mid);

  wait(0);
  say("parent: child finished\n");
  exit(0);
}

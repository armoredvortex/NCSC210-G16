#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int pid;

  printf("getppiddemo: parent pid=%d ppid=%d\n", getpid(), getppid());

  pid = fork();
  if(pid < 0){
    printf("getppiddemo: fork failed\n");
    exit(1);
  }

  if(pid == 0){
    printf("getppiddemo: child pid=%d ppid=%d\n", getpid(), getppid());
    exit(0);
  }

  wait(0);
  printf("getppiddemo: complete\n");
  exit(0);
}

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int child1, child2;
  int st1 = -1;
  int st2 = -1;

  printf("waitpiddemo: started\n");

  child1 = fork();
  if(child1 < 0){
    printf("waitpiddemo: first fork failed\n");
    exit(1);
  }

  if(child1 == 0){
    printf("child1(pid=%d): spawned\n", getpid());
    printf("child1(pid=%d): exiting with status 11\n", getpid());
    exit(11);
  }

  child2 = fork();
  if(child2 < 0){
    printf("waitpiddemo: second fork failed\n");
    kill(child1);
    wait(0);
    exit(1);
  }

  if(child2 == 0){
    printf("child2(pid=%d): spawned\n", getpid());
    pause(5);
    printf("child2(pid=%d): exiting with status 22\n", getpid());
    exit(22);
  }

  printf("parent: children created child1=%d child2=%d\n", child1, child2);
  printf("parent: waiting specifically for child1 pid=%d\n", child1);
  if(waitpid(child1, &st1) < 0){
    printf("parent: waitpid(child1) failed\n");
    kill(child2);
    wait(0);
    exit(1);
  }
  printf("parent: child1 finished, status=%d\n", st1);

  if(waitpid(child2, &st2) < 0){
    printf("parent: waitpid(child2) failed\n");
    exit(1);
  }
  printf("parent: child2 finished, status=%d\n", st2);

  printf("waitpiddemo: complete\n");
  exit(0);
}

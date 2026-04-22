#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int child1, child2;
  int st1 = -1;
  int st2 = -1;

  printf("waitpiddemo: started\n");
  printf("waitpiddemo: parent pid=%d\n", getpid());

  child1 = fork();
  if(child1 < 0){
    printf("waitpiddemo: first fork failed\n");
    exit(1);
  }

  if(child1 == 0){
    // Delay child1 so child2 is expected to finish first.
    pause(20);
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
    // Shorter delay than child1.
    pause(5);
    exit(22);
  }

  printf("parent: children created child1=%d child2=%d\n", child1, child2);
  printf("parent: child2 should exit earlier, but we call waitpid(child1) first\n");
  printf("parent: waiting specifically for child1 pid=%d\n", child1);

  if(waitpid(child1, &st1) < 0){
    printf("parent: waitpid(child1) failed\n");
    kill(child2);
    wait(0);
    exit(1);
  }
  printf("parent: waitpid(child1) returned, status=%d\n", st1);

  printf("parent: now waiting for child2 pid=%d\n", child2);
  if(waitpid(child2, &st2) < 0){
    printf("parent: waitpid(child2) failed\n");
    exit(1);
  }
  printf("parent: waitpid(child2) returned, status=%d\n", st2);

  printf("waitpiddemo: complete\n");
  exit(0);
}

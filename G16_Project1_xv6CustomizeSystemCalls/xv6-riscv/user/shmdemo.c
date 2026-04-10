#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int id;
  char *addr;

  printf("starting shared memory demo\n");

  // 1. Parent creates shared memory segment with key 42, size 4096 bytes
  id = shmget(42, 4096);
  if(id < 0){
    printf("shmget failed\n");
    exit(1);
  }
  printf("shmget(42, 4096) = %d\n", id);

  // 2. Parent attaches the segment
  addr = shmat(id);
  if(addr == (char*)-1){
    printf("shmat failed\n");
    exit(1);
  }
  printf("parent attached at %p\n", addr);

  // 3. Parent writes a message into shared memory
  char *msg = "hello from parent";
  char *dst = addr;
  while(*msg)
    *dst++ = *msg++;
  *dst = 0;
  printf("parent wrote: \"%s\"\n", addr);

  // 4. Fork a child
  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // --- CHILD ---
    // Child looks up the same segment by key
    int cid = shmget(42, 4096);
    if(cid < 0){
      printf("child shmget failed\n");
      exit(1);
    }
    printf("child shmget(42, 4096) = %d\n", cid);

    // Child attaches
    char *caddr = shmat(cid);
    if(caddr == (char*)-1){
      printf("child shmat failed\n");
      exit(1);
    }
    printf("child attached at %p\n", caddr);

    // Child reads parent's message
    printf("child reads: \"%s\"\n", caddr);

    // Child writes its own message
    char *cmsg = "hello from child";
    char *cdst = caddr;
    while(*cmsg)
      *cdst++ = *cmsg++;
    *cdst = 0;
    printf("child wrote: \"%s\"\n", caddr);

    // Child detaches
    shmdt(cid);
    printf("child detached\n");

    exit(0);
  } else {
    // --- PARENT ---
    // Wait for child to finish
    int status;
    wait(&status);

    // Parent reads child's message from shared memory
    printf("parent reads after child: \"%s\"\n", addr);

    // Parent detaches
    shmdt(id);
    printf("parent detached\n");

    
  }

  exit(0);
}

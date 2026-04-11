#define SBRK_ERROR ((char *)-1)

struct stat;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int waitpid(int, int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
int getppid(void);
int forkn(int);
int thread_create(void*, void*);
int mutex_create(void);
int mutex_lock(int);
int mutex_unlock(int);
int mutex_trylock(int);
int mutex_owner(int);
int freeze(int);
int thaw(int);
int signal_send(int, int);
int getprocstate(int);
char* sys_sbrk(int,int);
int pause(int);
int uptime(void);
int msgget(int key);    // for getting/creating a message queue
int sendmsg(int quid, int type, char* msg, int msglen); // for sending a message
int recvmsg(int quid, int type, char* buffer, int bufflen); // for receiving a message
int msgcount(int quid); // returns number of queued messages
int shmget(int key, int size);   // shared memory: get/create segment
char* shmat(int id);             // shared memory: attach segment
int shmdt(int id);               // shared memory: detach segment

#define SIG_TERM 1
#define SIG_STOP 2
#define SIG_CONT 3


// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
char* sbrk(int);
char* sbrklazy(int);

// printf.c
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));

// umalloc.c
void* malloc(uint);
void free(void*);

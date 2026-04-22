# G16 Project 1 Documentation (xv6 Custom System Calls)

## 1. Project Summary
This project extends xv6-riscv with custom system calls for synchronization, process control, IPC, and process introspection. It includes user-space demo programs for validating each feature inside xv6.

Repository root for the implementation: `xv6-riscv/`.

## 2. Team Members
- Rachit Kumar Pandey [24JE0677]
- Rahul Joshi [24JE0678]
- Raj Priyadarshi [24JE0679]
- Rajarshi Chakraborty [24JE0680]
- Ramala Karthik [24JE0681]
- Ranish Garg [24JE0682]

## 3. Features Implemented
### 3.1 Kernel Mutex Support
- Syscalls: `mutex_create`, `mutex_lock`, `mutex_unlock`
- Demo: `mutexdemo`
- Summary: Kernel-managed mutex table with ownership checks and blocking acquisition behavior.

### 3.2 Freeze/Thaw Process Control
- Syscalls: `freeze(pid)`, `thaw(pid)`
- Demo: `freezethawdemo`
- Summary: Temporarily pauses and resumes target processes using scheduler-aware process state.

### 3.3 Message Queue IPC
- Syscalls: `msgget`, `sendmsg`, `recvmsg`
- Demo: `msgdemo`
- Summary: Key-based queue creation/access and typed message passing with non-blocking receive.

### 3.4 Shared Memory IPC
- Syscalls: `shmget`, `shmat`, `shmdt`
- Demo: `shmdemo`
- Summary: Shared segments with page mapping, refcounting, and auto-cleanup on detach/exit.

### 3.5 Targeted Child Wait
- Syscall: `waitpid(int pid, int *status)`
- Demo: `waitpiddemo`
- Summary: Parent can wait for a specific child process instead of any child.

### 3.6 Parent PID Lookup
- Syscall: `getppid(void)`
- Demo: `getppiddemo`
- Summary: Returns caller parent PID with safe fallback behavior.

### 3.7 Batch Process Creation
- Syscall: `forkn(int n)`
- Demo: `advancedsysdemo`
- Summary: Creates multiple children in one syscall invocation.

### 3.8 Thread-Style Process Start
- Syscall: `thread_create(void *start, void *arg)`
- Demo: `advancedsysdemo`
- Summary: Starts execution from a function pointer with argument in user mode.

### 3.9 Non-Blocking Mutex Acquire
- Syscall: `mutex_trylock(int id)`
- Demo: `advancedsysdemo`
- Summary: Immediate lock attempt without sleeping.

### 3.10 Unified Signal Dispatch
- Syscall: `signal_send(int pid, int sig)`
- Demo: `advancedsysdemo`
- Summary: Unified entry point for TERM/STOP/CONT semantics.

### 3.11 Message Queue Depth Query
- Syscall: `msgcount(int qid)`
- Demo: `advancedsysdemo`
- Summary: Returns number of pending messages in a queue.

### 3.12 Mutex Ownership Introspection
- Syscall: `mutex_owner(int id)`
- Demo: `sys2demo`
- Summary: Reports PID owning the mutex, unlocked state, or invalid ID.

### 3.13 Process State Introspection
- Syscall: `getprocstate(int pid)`
- Demo: `sys2demo`
- Summary: Returns xv6 internal process state for a specific PID.

## 4. Build and Demo Run
From project root:
```bash
cd xv6-riscv
make clean && make qemu
```

Then run demos in xv6 shell as needed:
- `mutexdemo`
- `freezethawdemo`
- `msgdemo`
- `shmdemo`
- `waitpiddemo`
- `getppiddemo`
- `advancedsysdemo`
- `sys2demo`

## 5. Contribution Breakdown

- Rachit Kumar Pandey [24JE0677]
  - Feature 1: mutex syscall set (`mutex_create`, `mutex_lock`, `mutex_unlock`)
  - Feature 2: process control (`freeze`, `thaw`)

- Ramala Karthik [24JE0681]
  - Feature 3: message queue IPC (`msgget`, `sendmsg`, `recvmsg`)

- Ranish Garg [24JE0682]
  - Feature 4: shared memory IPC (`shmget`, `shmat`, `shmdt`)

- Raj Priyadarshi [24JE0679]
  - Feature 5: `waitpid`
  - Feature 6: `getppid`

- Rahul Joshi [24JE0678]
  - Feature 7: `mutex_owner`
  - Feature 8: `getprocstate`

- Rajarshi Chakraborty [24JE0680]
  - Feature 9: `forkn`
  - Feature 10: `thread_create`
  - Feature 11: `signal_send`

## 6. Screenshot

- Mutex demo:
  ![mutexdemo Output](screenshots/mutexdemo.png)
- Freeze/thaw demo:
  ![freezethawdemo Output](screenshots/freezethawdemo.png)
- Message queue demo:
  ![msgdemo Output](screenshots/msgdemo.png)
- Shared memory demo:
  ![shmdemo Output](screenshots/shmdemo.png)
- Waitpid demo:
  ![waitpid Output](screenshots/waitpid_demo.jpeg)
- Getppid demo:
  ![getppid Output](screenshots/getppid_demo.jpeg)
- Mutex_owner, getprocstate demo:
  ![Advanced Syscall Demo Output](screenshots/sys2demo.png)



# G16 Project 1 - xv6 System Calls

## Team Members
- Rachit Kumar Pandey [24JE0677]
- Rahul Joshi [24JE0678]
- Raj Priyadarshi [24JE0679]
- Rajarshi Chakraborty [24JE0680]
- Ramala Karthik [24JE0681]
- Ranish Garg [24JE0682]

## Features Implemented

### Feature 1
- Feature: Kernel mutex lock table
- Syscalls: mutex_create, mutex_lock, mutex_unlock
- Description: Creates mutex id in kernel and supports lock/unlock from user processes.
- The mutex table is maintained in kernel space with fixed entries, and each entry tracks whether it is used, locked, and current owner pid.
- mutex_create allocates a free slot and returns its mutex id so user programs can share the same lock id across processes.
- mutex_lock blocks the caller when the mutex is already held and wakes it when the lock becomes available.
- mutex_unlock validates ownership before releasing, which prevents another process from unlocking a mutex it does not own.
- This feature demonstrates safe synchronization and controlled access to shared critical sections in xv6.
- Added By: Rachit Kumar Pandey
- Demo Program: mutexdemo

### Feature 2
- Feature: Process freeze and thaw control
- Syscalls: freeze, thaw
- Description: Allows one process to temporarily pause another runnable process and later resume it.
- freeze(pid) marks the target process as frozen, and the scheduler skips that process until thaw is called.
- thaw(pid) clears the frozen state and allows the process to run again in normal scheduling.
- The implementation is integrated with process lifecycle logic so freeze state is reset on process reuse.
- The kill path also clears freeze state so killed processes are not stuck indefinitely.
- Added By: Rachit Kumar Pandey
- Demo Program: freezethawdemo

## How To Run Demo
1. cd xv6-riscv
2. make qemu
3. Run in xv6 shell: mutexdemo
4. Run in xv6 shell: freezethawdemo


## Feature 3 
- **Feature**: Message queue based IPC system 
- **Syscalls**:  
    1. msgget(int key)
    2. sendmsg(int quid, int type, char* msg, int msglen)
- **msgget**
    - ***input params***: 
        - integer key `(eg. msgget(123))`
    - ***returns***: 
        - success  message queue id `int` 
        - failure  `-1`
    - ***Kernel data structures created***: 
        - message (struct with data,type)
        - msgq (message queue struct with array of messages and other required data) 
        > for more information see [msg.c](xv6-riscv/kernel/msg.c) and [msg.h](xv6-riscv/kernel/msg.h)
    - ***description***:
        msgget allows processes to get/create a message queue with a integer key which allows inter process communication. if a queue with the given key already exists, it returns the existing queue id. otherwise assigns a new queue entry in the kernel
    - ***implementation details***:
        - a array of queues `msgq` is maintained to store   queues (type `msgqueue`)
        - each index of `msgq` is a queue (eg. msgq[0] is q0)
        - every queue contains a variable `used` to know whether queue is being allocated or not
        - `key` is used to identify the queue uniquely which is stored in each queue 
        - when msgget is called first unused queue is allocated and it's `id` (array idx) is returned 
        - spinlock `msgqlock` is used to prevent race conditions
-  **sendmsg**
    - ***input params***: 
        - queue id `int` 
        - message type `int`
        - message buffer `char*`
        - message length `int`
    - ***returns***: 
        -  0    `success`
        - -1    `failure`
    - ***description***: 
        `sendmsg` allows a user program to send a message (byte buffer) to a message queue with id quid. the receiving program must call recvmsg on the same queue to retrieve the message
    ***implementation details***: 
        - messages are stored in a fixed size array and `valid` flag is used to track active entries
        - uses copyin to safely transfer data from user space to kernel space 
        - uses spinlock `msgqlock` to enforce mutual exclusion while modifying queue state
        - returns error `-1` if queue is full or invalid queue id or message length exceed the limit (128 bytes)
        > max msg len: 128 bytes
- **recvmsg**
    - ***Input params***:
        - queue id `int`
        - type `int`
        - user buffer (where msg should be copied) `char*`
        - buffer len `int`
    - ***returns***:
        - success `copied msg len`
        - failure `-1`
    - ***description***: 
        - `recvmsg` allows a user program to retrieve the msg of desired type from the msg queue.
        user program must provide the queue id of the queue it want to receive message from.
        the implementation is non-blocking (returns right away -1 if no message of desired type found)
    - ***implementation details***: 
        - `recvmsg` is `non-blocking` (returns -1 right away if no message of desired type found)
        -  `copyout` is used to safely copy from kernel buffer to user buffer 
        - uses `valid` variable to track whether message is active
        - linearly searches the queue with provided quid for the desired message type
        - returns the first message found of the requested message type
        
## How To Run Demo
1. cd xv6-riscv
2. make clean && make qemu
3. Run in xv6 shell: msgdemo

        
    


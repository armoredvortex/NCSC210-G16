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
- **Syscalls**:  msgget(key)
- **msgget**
    - ***input params***: integer key `(eg. msgget(123))`
    - ***returns***: message queue id 
    - ***Kenel data structures created***: 
        - message (struct with msgdata,type) ,msgq(message queue struct with array of messages and other required data) 
        > for more information see [msg.c](xv6-riscv/kernel/msg.c) and [msg.h](xv6-riscv/kernel/msg.h)
    - ***description***:
        msgget allows processes to get/create a message queue with a integer key which allows inter process communication. if a queue with the given key already exists, it returns the existing queue id. otherwise assigns a new queue entry in the kernel
        the implementation uses spinlock`(msglock)` to avoid race conditions when multiple processes attempt to get/create queues concurrently
    


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

## How To Run Demo
1. cd xv6-riscv
2. make qemu
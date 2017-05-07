// Various variables and functions related to implementing system calls.

#ifndef _SYSCALL_H
#define _SYSCALL_H

// The number of syscalls that exist.
#define NUM_SYSCALLS 11
// The maximum number of arguments that a syscall can have.
#define MAX_SYSCALL_ARGS 6

#ifndef ASM

#include "types.h"

// Simply empty handler for system calls (int 0x80) that will later handle
// system calls by user programs. Implemented in syscall-asm.S.
void system_call_handler();

// Responsible for setting up all system calls to their default implementation
// by filling in the syscall_jump_table.
// Output: Alters syscall_jump_table.
void setup_syscalls();

// Basic function pointer type to represent a system call implementation. Note
// that an actual syscall impl can have as many arguments as desired.
typedef int32_t (*syscall_impl)(void);

// Adds new system calls to the syscall_jump_table.
//
// The %eax register is not restored so whatever remains after |impl| is run
// will be returned to the user. This is useful for syscalls that return
// values, but may leak information for syscalls that do not. Providing an
// already in use call_num will overwrite the previous syscall that was stored
// there (but also print out a warning).
//
// Input: |call_num| The number that the user provides in %eax to signify this
//                   syscall.
//        |impl| The function that implements this syscall.
// Output: Alters the syscall_jump_table to add the new syscall.
// Return: -1 if failure, 0 on success.
int32_t set_syscall(unsigned long syscall_num, void *impl);

// A single entry in the syscall jump table that respresents a single system
// call.
typedef struct __attribute__((packed)) {
    // The function that implements the syscall.
    syscall_impl impl;
} syscall_jt_entry;

// Jump/call table that stores implementation every system call.
extern syscall_jt_entry syscall_jump_table[NUM_SYSCALLS];

#endif // #ifndef ASM

#endif // #ifndef _SYSCALL_H

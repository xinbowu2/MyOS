#include "syscall.h"
#include "../syscalls/ece391sysnum.h"
#include "fs.h"
#include "lib.h"
#include "sys_execute.h"
#include "sys_getargs.h"
#include "sys_vidmap.h"
#include "sys_halt.h"

// Jump/call table that stores implementation every system call.
syscall_jt_entry syscall_jump_table[NUM_SYSCALLS];

// Simple syscall impl that represents an unimplemented syscall. Just prints
// out an error and returns -1.
// Output: Prints out an error message.
// Return: -1
static int32_t unimplemented_syscall()
{
    printf("Error: No such syscall.\n");
    return -1;
}

// Responsible for setting up all system calls by filling in the
// syscall_jump_table.
// Output: Alters syscall_jump_table.
void setup_syscalls()
{
    set_syscall(0, unimplemented_syscall);
    // int32_t halt (uint8 t status);
    set_syscall(SYS_HALT, halt);
    // int32_t execute (const uint8 t* command);
    set_syscall(SYS_EXECUTE, execute);
    // int32_t read (int32 t fd, void* buf, int32 t nbytes);
    set_syscall(SYS_READ, sys_read);
    // int32_t write (int32 t fd, const void* buf, int32 t nbytes);
    set_syscall(SYS_WRITE, sys_write);
    // int32_t open (const uint8 t* filename);
    set_syscall(SYS_OPEN, sys_open);
    // int32_t close (int32_t fd);
    set_syscall(SYS_CLOSE, sys_close);
    // int32_t getargs (uint8 t* buf, int32_t nbytes);
    set_syscall(SYS_GETARGS, getargs);
    // int32_t vidmap (uint8 t** screen start);
    set_syscall(SYS_VIDMAP, vidmap);
    // int32_t set_handler (int32 t signum, void* handler address);
    set_syscall(SYS_SET_HANDLER, unimplemented_syscall);
    // int32_t sigreturn (void);
    set_syscall(SYS_SIGRETURN, unimplemented_syscall);
}

// Adds new system calls to the syscall_jump_table.
//
// The %eax register is not restored so whatever remains after |impl| is run
// will be returned to the user. This is useful for syscalls that return
// values, but may leak information for syscalls that do not. Providing an
// already in use call_num will overwrite the previous syscall that was stored
// there.
//
// Input: |call_num| The number that the user provides in %eax to signify this
//                   syscall.
//        |impl| The function that implements this syscall.
//        |num_args| The number of arguments this function expects.
// Output: Alters the syscall_jump_table to add the new syscall.
// Return: -1 if failure, 0 on success.
int32_t set_syscall(unsigned long syscall_num, void *impl)
{
    if (syscall_num >= NUM_SYSCALLS) {
        printf("Error: Invalid syscall number %u.\n", syscall_num);
        return -1;
    }

    if (syscall_jump_table[syscall_num].impl) {
        printf("Warning: Overriding already existing syscall %u.\n", syscall_num);
    }

    syscall_jump_table[syscall_num].impl = (syscall_impl)impl;

    return 0;
}

#ifndef _PCB_H
#define _PCB_H

#include "fs.h"
#include "keyboard.h"

/*
 * Process Control Block (PCB) is a data structure in the operating system kernel
 * containing the information needed to manage a particular process.
 * The PCB is "the manifestation of a process in an operating system".
 */

#define MAX_FILES_PER_PROCESS 8
#define MAX_NUM_PROCESSES 8

#ifndef ASM
#include "stdbool.h"

#define KB 1024
#define MB (KB * 1024)
#define GB (MB * 1024)
#define KERNEL_STACK_SIZE (8 * KB)
#define KERNEL_PAGE_END (8 * MB)
/*
 * PCB[i] starts at 8MB - (pid + 1) * 8KB
 */
#define GET_PCB_ENTRY(pid) ((pcb_entry_t *)(KERNEL_PAGE_END - ((pid + 1) * KERNEL_STACK_SIZE)))

/* A single pcb entry that goes in the pcb */
typedef struct pcb_entry {
    // True means this pcb_entry is currently representing a process. False
    // means there is no process being stored here.
    bool active;
    // True if process should be run by the scheduler. Set to false to stop
    // scheduler from running this.
    bool runnable;
    // The terminal this process belongs to.
    int32_t terminal;
    // Jump back point for schedule().
    uint32_t sched_jump_back;

    /*
    * Each task can have up to 8 open files.
    * These open files are represented with a file array, stored in the process control block (PCB).
    * The integer index into this array is called the "file descriptor", and this integer is how user-level programs indentify the open file.
    */
    file_t files[MAX_FILES_PER_PROCESS];
    int myparent_pid; //do i still need parent pcb pointer?
    int my_pid;
    uint8_t arguments[keyboard_buf_size + 1]; //+1 to ensure that there is a room to put NULL at the end
    uint32_t program_entry;
    uint32_t esp;
    uint32_t esp0;
    uint32_t ebp;
    uint32_t child_status;
} pcb_entry_t;

// Process identifier of currently running process (not the currently visible
// one).
extern int curr_pid;

void pcb_init();

#endif //ASM
#endif //_PCB_H

#include "sys_halt.h"
#include "lib.h"
#include "sys_execute.h"

/*
Take the current process and close the file it opens, after it calculates which pcb where are at.
However, it needs to consider whether or not it is a shell. If it is a shell, start another shell;
If it is not, return to the parent process. (current pid --> parent:pid, stack, esp0; set the esp ->esp[0])
Make the page again for the parent process; change the current pid to the parent's pid (might not matter);
reset the esp and ebp value, and move the status (halt's parameter) of the program, put the
status to the eax; status is the error if there is one. (store esp and ebp to pcb)
*/
int32_t internal_halt(uint32_t status)
{
    cli();

    pcb_entry_t *current_pcb = GET_PCB_ENTRY(curr_pid);

    // No matter what, disable PCB and stop from running.
    current_pcb->active = false;
    current_pcb->runnable = false;

    // TODO: Think about whether this will work with scheduling and terminal switching.
    if (current_pcb->myparent_pid == -1) {
        printf("Shell has no parent to return to, so just executing another shell\n");
        // Start off curr_pid at -1 meaning next execute() will be first process.
        curr_pid = -1;
        execute((uint8_t *)"shell");

        // Should never run, but just in case.
        return -1;
    }

    pcb_entry_t *parent_pcb = GET_PCB_ENTRY(current_pcb->myparent_pid);
    int i;

    // Enable running parent process.
    parent_pcb->runnable = true;

    /* Save the 8 bits status to the 8 bits ret-val entry in the parent */
    parent_pcb->child_status = status;

    /* Close the open files */
    for (i = 0; i < MAX_FILES_PER_PROCESS; i++) {
        if ((current_pcb->files[i]).flags != AVAILABLE) {
            sys_close(i);
        }
    }

    /* Change the page directory to the parent's page directory */
    SET_CR3(&pds[current_pcb->myparent_pid]);

    /* Zeroing out page directory */
    memset(&pds[curr_pid], 0x00, sizeof(page_directory_t));

    /* Restore tss.esp0 to parent's tss.esp0 */
    tss.esp0 = parent_pcb->esp0;

    /* Update current_pid */
    curr_pid = current_pcb->myparent_pid;

    /* Restore esp, ebp and eip to the parent's value */
    asm volatile(
            "movl %0, %%esp;"
            "movl %1, %%ebp;"
            "jmp EXE_RETURN;"
            :
            : "g"(parent_pcb->esp), "g"(parent_pcb->ebp)
            : "esp", "ebp");

    return -1; /*A fake return. It should never be executed*/
}

/*
 * Version of halt that is called by userspace programs when they make syscall.
 * Truncates status (return value) to 8 bit integer. Otherwise, exactly the
 * same as internal_halt().
 */
int32_t halt(uint8_t status)
{
    return internal_halt(status);
}

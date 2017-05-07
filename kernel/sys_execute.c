#include "sys_execute.h"

#include "fs.h"
#include "lib.h"
#include "pcb.h"
#include "pt.h"
#include "x86_desc.h"
#include "keyboard.h"
#include "sys_vidmap.h"
#include "schedule.h"
/*
 * The execute system call attempts to load and exeute a new program,
 * handing off the proessor to the new program until it terminates.
 * The command is a space-separated sequence of words.
 * The first word is the file name of the program to be executed,
 * and the rest of the command stripped of leading spaces should be provided
 * to the new program on request via the getargs system call.
 * The execute call returns -1 if the command cannot be executed,
 * 256 if the program dies by an exception,
 * or a value in the range 0 to 255 if the program exeutes a halt system call,
 * in which case the value returned is that given by the program's call to halt.
*/
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

//The first 4 bytes of the file represent a "magic number" that identies the file as an exeutable.
#define EXE_MAGIC_NUMBER_LENGTH 4
// These 4 bytes are, respectively, 0: 0x7f; 1: 0x45; 2: 0x4c; 3: 0x46.
// Reversed for little endian
#define EXE_MAGIC_NUMBER 0x464c457f

/*
 * The other important bit of information that you need to execute programs is the entry point into the program,
 * i.e., the virtual address of the first instruction that should be executed.
 * This information is stored as a 4-byte unsigned integer in bytes 24-27 of the exeutable,
 * and the value of it falls somewhere near 0x08048000 for all programs we have provided to you.
 */
#define ENTRY_MAGIC_INDEX 24

/* Bottom of user program page (virtual address) minus 4 */
/* Minus 4 to avoid dereferencing the next page */
#define USER_STACK (128 * MB + 4 * MB - 4)

// TODO: Add new version of execute (with additional argument) for starting on different terminal.
/*
 * Executes the program named |command| then returns result to caller.
 * Input: C string containing the command to run (executable name and
 *        arguments).
 * Output: Executes the desired program and then returns the program's return
 *         value.
 * Return: 0-255 for return code of program. -1 if couldn't find program or if
 *         reached max process limit. 256 if exception occurred during program
 *         execution.
 */
int32_t execute(const uint8_t *command)
{
    cli();

    uint8_t *iterator;
    uint8_t file_name[FILE_NAME_LENGTH];
    uint8_t args_cpy[keyboard_buf_size + 1];
    uint32_t magic_number;
    int32_t entry_addr; /*entry address of a program (virtual)*/
    int parent_pid = curr_pid;
    pcb_entry_t *parent_pcb = GET_PCB_ENTRY(parent_pid);
    uint32_t process_physical_addr;
    dentry_t dentry;
    int i, num_times;

    int next_pid = -1;
    pcb_entry_t *next_pcb = NULL;
    // Need complex logic to handle when curr_pid == -1.
    for (i = 0, num_times = (curr_pid == -1 ? MAX_NUM_PROCESSES : MAX_NUM_PROCESSES - 1);
         i < num_times; i++) {
        int pid = (curr_pid + 1 + i) % MAX_NUM_PROCESSES;
        next_pcb = GET_PCB_ENTRY(pid);
        if (!next_pcb->active) {
            next_pid = pid;
            break;
        }
    }
    if (next_pid == -1) {
        printf("Already at maximum number of processes.\n");
        return -1;
    }

    iterator = (uint8_t *)command;
    /* Skip over leading spaces */
    while (*iterator == ' ' && *iterator != '\0' && *iterator != '\n') {
        iterator++;
    }
    command = iterator;

    /* Find first space after command */
    while (*iterator != ' ' && *iterator != '\0' && *iterator != '\n') {
        iterator++;
    }

    /* Copy over and null terminate */
    strncpy((int8_t *)file_name, (int8_t *)command, MIN(iterator - command, FILE_NAME_LENGTH));
    file_name[MIN(iterator - command, FILE_NAME_LENGTH)] = '\0';

    /* Advance to first argument */
    while (*iterator == ' ' && *iterator != '\0' && *iterator != '\n') {
        iterator++;
    }

    uint8_t* args_begin = iterator;
    while (*iterator != '\0' && *iterator != '\n') {
        iterator++;
    }

    /* Everything else is args for getargs() */
    strncpy((int8_t *)args_cpy, (int8_t *)args_begin, MIN(iterator - args_begin , sizeof(args_cpy)));
    args_cpy[MIN(iterator - args_begin , sizeof(args_cpy))] = '\0';

    //printf("file_name: %s, args: %s\n", file_name, args_cpy);

    /*check file validity*/
    /*check if the file exists*/
    if (read_dentry_by_name(file_name, &dentry) == -1) {
        return -1;
    }

    /*
     * The first 4 bytes of the file represent a magic number that identies the file as an executable.
     * These bytes are, respetively, 0: 0x7f; 1: 0x45; 2: 0x4c; 3: 0x46.
     */
    if (read_data(dentry.inode_number, 0, (void *)&magic_number, EXE_MAGIC_NUMBER_LENGTH) < EXE_MAGIC_NUMBER_LENGTH) {
        return -1;
    }

    /* If the magic number is not present, the execute system call should fail. */
    if (magic_number != EXE_MAGIC_NUMBER) {
        return -1;
    }

    // printf("magic number: %x\n", magic_number);

    /*
     * The other important bit of information that you need to execute programs is the entry point into the program,
     * i.e., the virtual address of the first instruction that should be executed.
     * This information is stored as a 4-byte unsigned integer in bytes 24-27 of the exeutable,
     * and the value of it falls somewhere near 0x08048000 for all programs we have provided to you.
     * Size of entry address is 4 bytes.
     */
    read_data(dentry.inode_number, ENTRY_MAGIC_INDEX, (void *)&entry_addr, 4);
    // printf("entry_addr: %x\n", entry_addr);

    /*update pid*/
    curr_pid = next_pid;

    /*Load file into memory*/
    /*
     * Your code should make a note of the entry point,
     * and then copy the entire file to memory starting at virtual address 0x08048000.
     */
    /* The first user-level program (the shell) should be loaded at physical address (8 MB)*/
    /* The second user-level program (excecuted by the shell) should be loaded at physical address (12 MB)*/
    process_physical_addr = PID_TO_PHYSICAL_ADDRESS(next_pid);

    /* Setup the user process page directory */
    /* Zeroing out page directory */
    memset(&pds[next_pid], 0x00, sizeof(page_directory_t));
    /* Mapping the kernel for the process*/
    map_page_directory_entry(&pds[next_pid], KERNEL_MEMORY, KERNEL_MEMORY, P | PS | G);
    /*
    *The program image itself is linked to execute at virtual address 0x08048000.
    * The way to get this working is to set up a single 4 MB page directory entry that maps virtual address 0x08000000 (128 MB)
    * to the right physical memory address (either 8 MB or 12 MB).
    * Then, the program image must be copied to the correct offset (0x00048000) within that page.
    */
    map_page_directory_entry(&pds[next_pid], PROGRAM_VIRTUAL_ADDRESS, process_physical_addr, P | PS | US | RW);

    // TODO: Need to handle mapping video to correct location based on terminal.

    next_pcb->terminal = visible_terminal;
    if (parent_pid >= 0) {
        next_pcb->terminal = parent_pcb->terminal;
    }
    /* Whereas the first 4 MB of memory should broken down into 4 kB pages.*/
    /* Mapping first 4 MB of memory to a page table that breaks the 4mb of memory into 4k pages */
    map_page_directory_entry(&pds[next_pid], VIDEO_MEMORY, (uint32_t)us_vid_mem_pt[next_pcb->terminal], P | RW);

    /*TLB flushes automatically when CR3 is updated*/
    SET_CR3(&pds[next_pid]);

    // printf("process_physical_addr: 0x%x\n", process_physical_addr);
    /* The program image must be opied to the correct offset (0x00048000) within that page. */
    // 4MB is the upper bound on an executable
    read_data(dentry.inode_number, 0, (void *)(PROGRAM_VIRTUAL_ADDRESS), 4 * MB);

    /*Create kernel stack for each process*/
    next_pcb->active = true;
    next_pcb->runnable = true;
    next_pcb->myparent_pid = parent_pid;
    next_pcb->my_pid = next_pid;
    next_pcb->program_entry = entry_addr; // remove later

    /*clear the flags in the file array of the current pcb*/
    for (i = 0; i < MAX_FILES_PER_PROCESS; i++) {
        (next_pcb->files[i]).flags = AVAILABLE;
    }

    /* Open stdin and stdout */
    open_stdin();
    open_stdout();

    //printf("args in execute:%s\n", args_cpy);
    strncpy((int8_t *)next_pcb->arguments, (int8_t *)args_cpy, sizeof(next_pcb->arguments));
    next_pcb->arguments[sizeof(next_pcb->arguments) - 1] = '\0';

    // If not first process, start saving stuff for later return and stop
    // running parent.
    if (parent_pid >= 0) {
        // Stop parent from running.
        parent_pcb->runnable = false;
        /* Don't make any function calls after this point b/c saved %esp won't be accurate anymore */
        /*Save ESP and EBP registers to the current pcb*/
        STORE_REGISTER_VALUE(esp, &parent_pcb->esp);
        STORE_REGISTER_VALUE(ebp, &parent_pcb->ebp);
    }

    /*tss.SS0 points to the kernel's stack segment (-4)*/
    tss.ss0 = KERNEL_DS;
    /*Alter the TSS entry in order to be able to IRET back here from the user program*/
    /*tss.ESP0 points to the start of the current process's kernel-mode stack*/
    // 4 is the size of first element on the stack
    tss.esp0 = KERNEL_PAGE_END - (KERNEL_STACK_SIZE * next_pid) - 4;
    next_pcb->esp0 = tss.esp0;

//    asm volatile(
//            "pushl  %%edi;"
//            "pushl  %%esi;"
//            "pushl  %%edx;"
//            "pushl  %%ecx;"
//            "pushl  %%ebx;"
//            "pushl  %%eax;"
//    :);

    /*push the right value for the user-level registers to prepare for IRET to the user program*/
    /* Order determined in x86 manual for IRET */
    /*SS <- stack sagment in GDT */
    pushl(USER_DS);
    /*ESP <- user stack pointer points to the bottom of the page (user virtual adress)*/
    pushl(USER_STACK);
    /*EFLAG (pushl EFLAGS)*/
    /*0x200 is for setting the IF flag in EFLAGS*/
    asm volatile(
            "pushf;"
            "orl $0x200, (%%esp)" ::
                    : "esp", "memory");
    //pushfl();
    /*CS <- user-mode code segment in GDT*/
    pushl(USER_CS);
    /*EIP <- program entry*/
    // pushl entry_addr
    asm volatile("pushl %0;"
                 :
                 : "g"(entry_addr)
                 : "memory"); /*DS <- user mode data segment in GDT*/
                              //movl USER_DS DS
    asm volatile("movl $" STRINGIFY2(USER_DS) ", %%eax;"
                         "movl %%eax, %%ds;"
                 :
                 :
                 : "memory", "eax"); /*set CR3*/

    asm volatile("iret");

    asm volatile(".globl EXE_RETURN");
    asm volatile("EXE_RETURN: ");

//    asm volatile(
//            "popl  %%eax;"
//            "popl  %%ebx;"
//            "popl  %%ecx;"
//            "popl  %%edx;"
//            "popl  %%esi;"
//            "popl  %%edi;"
//    :);

    // printf("child_status: %d\n", parent_pcb->child_status);
    return parent_pcb->child_status;
}

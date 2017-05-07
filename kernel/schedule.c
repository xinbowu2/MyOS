#include "schedule.h"

#include "pcb.h"
#include "x86_desc.h"
#include "pt.h"
#include "sys_execute.h"
#include "keyboard.h"
#include "terminal.h"
#include "i8259.h"

int32_t visible_terminal = 0;
terminal_components_t myTerminals[MAX_TERMINAL];
//There are three video page table, one for each terminal.
//When user process starts, points to the video paget table of the terminal it belongs to
//Each terminal page table points to either VGA memory or the invisible video page belonging
//to that terminal.
//When switching visible terminal, copy VGA memory to the invisible video page. Redirect
//the video page table to the invisible video page. Then copy the new terminal's video
//page to the VGA, and redirect the new terminal's video page table to VGA.

// Remembers whether a shell was opened on this terminal or not.
bool terminal_started_shell[MAX_TERMINAL] = {true, false, false};

// Saves the 'terminal' state
void save_terminal_state(int terminal) {
    // /* Save current cursor position.*/
    // myTerminals[terminal].cursor_row = get_cursor_row();
    // myTerminals[terminal].cursor_col = get_cursor_col();
    // /* Save flags for special keys */
    // myTerminals[terminal].caps_lock = Keyboard.caps_lock;
    // myTerminals[terminal].shift = Keyboard.shift;
    // myTerminals[terminal].alt = Keyboard.alt;
    // myTerminals[terminal].control = Keyboard.control;
    // myTerminals[terminal].buf_index = Keyboard.buf_index;
    // myTerminals[terminal].keyboard_buff_to_go = Keyboard.keyboard_buff_to_go;
    // /* Save current keyboard buffer to safe place for current terminal.*/
    // memcpy(myTerminals[terminal].keyboard_buf, get_keyboard_in_buf(), myTerminals[terminal].buf_index);
    /* Save current video buffer to safe place for current terminal.*/
    uint32_t invisible_video_memory = VIDEO_MEMORY + VIDEO_MEMORY_SIZE * (terminal + 1);
    memcpy((void *)invisible_video_memory,(void *)VIDEO_MEMORY, VIDEO_MEMORY_SIZE);
    /* Redirect current terminal's entry in video page table to some safe place.*/
    map_page_table_entry((page_table_t *)us_vid_mem_pt[visible_terminal], VIDEO_MEMORY, invisible_video_memory, P | RW | US);
}

// Restores the 'terminal' state
void restore_terminal_state(int terminal) {
    /* Restore new terminal's cursor position.*/
    terminal_cursor(get_cursor_row_index(terminal), get_cursor_col_index(terminal));
    // /* Restore flags for special keys */
    // Keyboard.caps_lock = myTerminals[terminal].caps_lock;
    // Keyboard.shift = myTerminals[terminal].shift;
    // Keyboard.alt = myTerminals[terminal].alt;
    // Keyboard.control = myTerminals[terminal].control;
    // Keyboard.buf_index = myTerminals[terminal].buf_index;
    // Keyboard.keyboard_buff_to_go = myTerminals[terminal].keyboard_buff_to_go;
    // /* Restore keyboard buffer*/
    // memcpy(get_keyboard_in_buf(), myTerminals[terminal].keyboard_buf, myTerminals[terminal].buf_index);
    /* Restore video memory */
    uint32_t new_invisible_video_memory = VIDEO_MEMORY + VIDEO_MEMORY_SIZE * (terminal + 1);
    memcpy((void *)VIDEO_MEMORY, (void *)new_invisible_video_memory, VIDEO_MEMORY_SIZE);
    /* Redirect new visible terminals entry in video page table to VGA buf. */
    map_page_table_entry((page_table_t *)us_vid_mem_pt[terminal], VIDEO_MEMORY, VIDEO_MEMORY, P | RW | US);
}


int32_t switch_visible_terminal(int32_t new_terminal)
{
    if (new_terminal == visible_terminal) {
        return 0;
    }
    unsigned long flags;
    cli_and_save(flags);

    // Use kernel page table to make things easier.
    SET_CR3(&pd_kernel[0]);

    // save visible terminal state
    save_terminal_state(visible_terminal);

    // Restore new terminal's state
    restore_terminal_state(new_terminal);

    /*update visible terminal number*/
    visible_terminal = new_terminal;

    // Switch back to the user program pd that called this function.
    SET_CR3(&pds[curr_pid]);

    if (!terminal_started_shell[new_terminal]) {
        int pid;
        for (pid = 0; pid < MAX_NUM_PROCESSES; pid++) {
            if (!GET_PCB_ENTRY(pid)->active) {
                break;
            }
        }
        if (pid == MAX_NUM_PROCESSES) {
            SET_CR3(&pd_kernel[0]);
            printf("Can't start new terminal because out of processes.\n");
            SET_CR3(&pds[curr_pid]);
            return -1;
        }

        terminal_started_shell[new_terminal] = true;

        pcb_entry_t *curr_pcb = GET_PCB_ENTRY(curr_pid);

        // Need to enable all IRQs because switching contexts.
        // System timer.
        enable_irq(0);
        // Keyboard.
        enable_irq(1);
        // RTC.
        enable_irq(8);

//        asm volatile(
//                "movl $switch_term_jump_back, (%0);"
//            :: "g"(&curr_pcb->sched_jump_back)
//            );

        curr_pid = -1;

//        asm volatile(
//                "pushl  %%edi;"
//                "pushl  %%esi;"
//                "pushl  %%edx;"
//                "pushl  %%ecx;"
//                "pushl  %%ebx;"
//                "pushl  %%eax;"
//        :);

        // Save kernel stack registers (%esp, %ebp) in current PCB.
        STORE_REGISTER_VALUE(esp, &curr_pcb->esp);
        STORE_REGISTER_VALUE(ebp, &curr_pcb->ebp);
        execute((uint8_t*)"shell");

//        asm volatile("switch_term_jump_back:");
//        asm volatile(
//                "popl  %%eax;"
//                "popl  %%ebx;"
//                "popl  %%ecx;"
//                "popl  %%edx;"
//                "popl  %%esi;"
//                "popl  %%edi;"
//        :);

    }

    // TODO: Need to handle starting new shell for first time (or do it in keyboard handler).
    restore_flags(flags);
    return 0;
}

void schedule()
{
    cli();

    // Find next process (or return immediately if none)
    pcb_entry_t *curr_pcb = GET_PCB_ENTRY(curr_pid);

    int next_pid = -1;
    pcb_entry_t *next_pcb = NULL;
    int pid;
    for (pid = (curr_pid + 1) % MAX_NUM_PROCESSES;
         pid != curr_pid;
         pid = (pid + 1) % MAX_NUM_PROCESSES) {
        next_pcb = GET_PCB_ENTRY(pid);
        if (next_pcb->runnable) {
            next_pid = pid;
            //printf("sched from %d to %d\n", curr_pid, next_pid);
            break;
        }
    }
    if (next_pid == -1) {
        return;
    }

    // Need to enable all IRQs because switching contexts.
    // System timer.
    enable_irq(0);
    // Keyboard.
    enable_irq(1);
    // RTC.
    enable_irq(8);

    tss.esp0 = next_pcb->esp0;

    SET_CR3(&pds[next_pid]);

    curr_pid = next_pid;

    // TODO: See if necessary to save all registers locally.

//    asm volatile(
//        "movl $sched_jump_back, (%0);"
//        :: "g"(&curr_pcb->sched_jump_back)
//        );

//    asm volatile(
//            "pushl  %%edi;"
//            "pushl  %%esi;"
//            "pushl  %%edx;"
//            "pushl  %%ecx;"
//            "pushl  %%ebx;"
//            "pushl  %%eax;"
//    :);

    // Save kernel stack registers (%esp, %ebp) in current PCB.
    STORE_REGISTER_VALUE(esp, &curr_pcb->esp);
    STORE_REGISTER_VALUE(ebp, &curr_pcb->ebp);

    // Switch stacks to new process's version of schedule.
    asm volatile(
            "movl %0, %%esp;"
            "movl %1, %%ebp;"
//            "jmp  *%2;"
            "sched_jump_back:"
        ::"g"(next_pcb->esp), "g"(next_pcb->ebp)//, "g"(next_pcb->sched_jump_back));
    );

//    asm volatile(
//            "popl  %%eax;"
//            "popl  %%ebx;"
//            "popl  %%ecx;"
//            "popl  %%edx;"
//            "popl  %%esi;"
//            "popl  %%edi;"
//    :);


    // TODO: Restore original process's registers if necessary.
}

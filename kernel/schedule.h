/**
 * Handles terminal switching and scheduling switching.
 */

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

//#include "stdint.h"
#include "pt.h"
#include "sys_vidmap.h"
#include "types.h"
#include "pcb.h"

#define MAX_NUM_TERMINALS 3

#define TERMINAL_INDEX (((pcb_entry_t *)GET_PCB_ENTRY(curr_pid))->terminal)

//Components needed by a terminal
typedef struct terminal_components{

int cursor_row, cursor_col;
char keyboard_buf[keyboard_buf_size];
int caps_lock;
int shift;
int alt;
int control;
int buf_index;
int keyboard_buff_to_go;
} terminal_components_t;


/* Identifies which terminal is currently visible [0-MAX_NUM_TERMINALS].*/
extern int32_t visible_terminal;

// stores state for each terminal
extern terminal_components_t myTerminals[MAX_TERMINAL];

// Switches the currently visible terminal to the new one. |new_terminal|
// should range from [0-MAX_NUM_TERMINALS). Note that this does not change the
// currently running process.
// Input: The ID of the new terminal to switch to (0 to MAX_NUM_TERMINALS-1).
// Output: Changes the video memory, page tables, etc. to switch to new
//         terminal.
// Return: 0 on success, -1 on failure.
int32_t switch_visible_terminal(int32_t new_terminal);

// Call to trigger possibly switching to new current process. This will return
// when the calling process is chosen to be run (perhaps after others have been
// chosen). This function identifies runnable processes by the |runnable| field
// in the PCB.
void schedule();

#endif // #ifndef _SCHEDULE_H

#include "pcb.h"

pcb_entry_t pcb[MAX_NUM_PROCESSES];

pcb_entry_t *curr_pcb_entry = &pcb[0];

int curr_pid = -1;

void pcb_init()
{
    return;
}

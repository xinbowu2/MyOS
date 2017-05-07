#include "sys_execute.h"
#include "fs.h"
#include "lib.h"
#include "pcb.h"
#include "pt.h"
#include "x86_desc.h"

/*
 * getargs
 *   DESCRIPTION: Read the program's command line arguments into a user-level buffer
 *   INPUTS: buf -- buffer storing arguments
 * 			 nbytes -- size of the buffer
 *   OUTPUTS: none
 *   RETURN VALUE:  0, successful
 *				   -1, if the arguments cannot fit into the buffer
 *   SIDE EFFECTS: none
 */
int32_t getargs(uint8_t* buf, int32_t nbytes){
	pcb_entry_t *curr_pcb = GET_PCB_ENTRY(curr_pid);

	/*fail, if the arguments do not fit into the buffer*/
	if (nbytes < sizeof(curr_pcb->arguments))
		return -1;

	int i;
	for (i = 0; i < sizeof(curr_pcb->arguments); i++){
		if (curr_pcb->arguments[i] == '\0'){
			*(buf) = '\0';
			break;
		}


		*(buf++) = curr_pcb->arguments[i];
	}

	return 0;
}

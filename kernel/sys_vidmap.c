#include "types.h"
#include "sys_vidmap.h"
#include "pt.h" 
#include "pcb.h"
#include "lib.h"


pte_t us_vid_mem_pt0[NUM_PTE_ENTRIES] __attribute__((aligned(sizeof(page_table_t))));
pte_t us_vid_mem_pt1[NUM_PTE_ENTRIES] __attribute__((aligned(sizeof(page_table_t))));
pte_t us_vid_mem_pt2[NUM_PTE_ENTRIES] __attribute__((aligned(sizeof(page_table_t))));

page_table_t * us_vid_mem_pt[MAX_TERMINAL] = {
    (page_table_t*)us_vid_mem_pt0,
    (page_table_t*)us_vid_mem_pt1,
    (page_table_t*)us_vid_mem_pt2
};


/*
 * vidmap
 *   DESCRIPTION: map the text-mode video memory into user space at a pre-set virtual address
 *   INPUTS: screen_start -- Pointer to start of the screen
 *   OUTPUTS: none
 *   RETURN VALUE:  the pre-set virtual address for success
 *				   -1, if the input is invalid
 *   SIDE EFFECTS: none
 */
int32_t 
vidmap(uint8_t** screen_start){
	/*Sanity check*/
	//?how to check if it falls in the range of the user-level page

	if (screen_start == NULL || !is_user((uint32_t)screen_start))
		return -1;


	/*map phisical video memory to a pre-set virtual memory, which we set to 132MB*/

	/*get the page directory for the current process*/
	page_directory_t* curr_pd = &pds[curr_pid]; 

	/*get the page table for the current process according to the terminal it belongs to*/
    pcb_entry_t *curr_pcb_entry = GET_PCB_ENTRY(curr_pid);
	us_vid_mem_pt_ptr = us_vid_mem_pt[curr_pcb_entry->terminal];


    // TODO: Need to correctly handle mapping video to correct terminal video page.

	/*make the mapping of page table in current page directory*/
	map_page_directory_entry(curr_pd, VIR_VIDEO_MEMORY, (uint32_t)us_vid_mem_pt_ptr, P | RW | US);

    /* mapping video memory for the kernel */
    //map_page_table_entry((page_table_t *)us_vid_mem_pt_ptr, VIR_VIDEO_MEMORY, VIDEO_MEMORY, P | RW | US);

	SET_CR3(&pds[curr_pid]);

	*screen_start = (uint8_t*)VIR_VIDEO_MEMORY;

	return VIR_VIDEO_MEMORY;
}


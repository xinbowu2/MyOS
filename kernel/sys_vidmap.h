#ifndef _SYS_VIMAP_H
#define _SYS_VIMAP_H

#include "types.h"
#include "pt.h" 
#include "terminal.h"

#define  VIR_VIDEO_MEMORY (0x8400000 + VIDEO_MEMORY)

/*Page table for the page size of 4KB*/
extern page_table_t * us_vid_mem_pt[MAX_TERMINAL];

extern pte_t us_vid_mem_pt0[NUM_PTE_ENTRIES]__attribute__((aligned(sizeof(page_table_t))));
extern pte_t us_vid_mem_pt1[NUM_PTE_ENTRIES]__attribute__((aligned(sizeof(page_table_t))));
extern pte_t us_vid_mem_pt2[NUM_PTE_ENTRIES]__attribute__((aligned(sizeof(page_table_t))));

page_table_t* us_vid_mem_pt_ptr;

/*map the text-mode video memory into user space at a pre-set virtual address*/
int32_t vidmap(uint8_t** screen_start);


#endif /*_SYS_VIMAP_H*/

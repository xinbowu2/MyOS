// This file contains implementation for page directory/table.

#include "pt.h"
#include "sys_vidmap.h"
#include "schedule.h"

/* Maps a virtual address to a physical address in a page directory with the correct flags */
void map_page_directory_entry(page_directory_t *page_directory, uint32_t virtual_memory, uint32_t physical_memory, uint32_t flags)
{
    bool page_size = GET_PS(flags);
    uint32_t index = virtual_memory >> NUM_4MB_OFFSET_BITS;
    pde_t *entry = &page_directory->entries[index];
    *entry = *entry | flags;
    if (page_size) {
        SET_4MB_PDE_ADDRESS(*entry, physical_memory);
    } else {
        SET_4KB_PDE_ADDRESS(*entry, physical_memory);
    }
}

/* Maps a virtual address to a physical address in a page table with the correct flags */
void map_page_table_entry(page_table_t *page_table, uint32_t virtual_memory, uint32_t physical_memory, uint32_t flags)
{
    uint32_t index = (virtual_memory & 0x3FFFFF) >> NUM_4KB_OFFSET_BITS;
    pte_t *entry = &page_table->entries[index];
    *entry = *entry | flags;
    SET_PTE_ADDRESS(*entry, physical_memory);
}

/*
 * is_user
 *   DESCRIPTION: Check if a virtual address is in a user page
 *   INPUTS: virtual_memory -- The virtual memory to be checked
 *   OUTPUTS: none
 *   RETURN VALUE:  0, if the virtual address is not in a user page
 *                  1, if the virtual address is in a user page
 *   SIDE EFFECTS: none
 */
uint8_t is_user(uint32_t virtual_memory)
{   
    page_directory_t* curr_pd = &pds[curr_pid]; 

    uint32_t index = virtual_memory >> NUM_4MB_OFFSET_BITS;
    pde_t *pd_entry = &curr_pd->entries[index];



    if (!GET_US(*pd_entry))
        return 0;

    if(!GET_PS(*pd_entry)){
        page_table_t* page_table = (page_table_t*)GET_4KB_PDE_ADDRESS(*pd_entry);
        printf("pd_entry:%x\n", pd_entry);

        index = (virtual_memory & 0x3FFFFF) >> NUM_4KB_OFFSET_BITS;
        printf("page_table:%x\n", page_table);

        pte_t *pt_entry = &page_table->entries[index];

        printf("pt_entry:%x\n", pt_entry);

        if (!GET_US(*pt_entry))
            return 0;

        printf("is_user:here");
    }

    return 1;
}

// Inititialize the page directory and table. Sets up page directory entry for
// kernel memory (0x400000) and for initial video memory.
void page_table_init()
{
    pde_t *kernel_pd = (pde_t *)&pd_kernel[0];
    int pid;
    int i;

    /* Setting up static page tables */
    /* The kernel is loaded at physial address 0x400000 (4 MB),
     * and also mapped at virtual address 4 MB.
     * A global page directory entry with its Supervisor bit set
     * should be set up to map the kernel to virtual address 0x400000 (4 MB).
     */
    map_page_directory_entry((page_directory_t *)kernel_pd, KERNEL_MEMORY, KERNEL_MEMORY, P | PS | G);

    /* Whereas the first 4 MB of memory should broken down into 4 kB pages.*/
    /* Mapping first 4 MB of memory to a page table that breaks the 4mb of memory into 4k pages */
    map_page_directory_entry((page_directory_t *)kernel_pd, 0, (uint32_t)vid_mem_pt, P);

    /* mapping video memory for the kernel */
    /*VGA memory*/
    map_page_table_entry((page_table_t *)vid_mem_pt, VIDEO_MEMORY, VIDEO_MEMORY, P);
    /*invisible video page for terminal 1*/
    map_page_table_entry((page_table_t *)vid_mem_pt, VIDEO_MEMORY + VIDEO_MEMORY_SIZE, VIDEO_MEMORY + VIDEO_MEMORY_SIZE, P);
    /*invisible video page for terminal 2*/
    map_page_table_entry((page_table_t *)vid_mem_pt, VIDEO_MEMORY + VIDEO_MEMORY_SIZE * 2, VIDEO_MEMORY + VIDEO_MEMORY_SIZE * 2, P);
    /*invisible video page for terminal 3*/
    map_page_table_entry((page_table_t *)vid_mem_pt, VIDEO_MEMORY + VIDEO_MEMORY_SIZE * 3, VIDEO_MEMORY + 3 * VIDEO_MEMORY_SIZE, P);

    // Setup terminal page tables.
    for (i = 1; i <= MAX_NUM_TERMINALS; i++) {
        map_page_table_entry((page_table_t *)us_vid_mem_pt[0], VIDEO_MEMORY + i * VIDEO_MEMORY_SIZE, VIDEO_MEMORY + i * VIDEO_MEMORY_SIZE, P | RW);
        map_page_table_entry((page_table_t *)us_vid_mem_pt[1], VIDEO_MEMORY + i * VIDEO_MEMORY_SIZE, VIDEO_MEMORY + i * VIDEO_MEMORY_SIZE, P | RW);
        map_page_table_entry((page_table_t *)us_vid_mem_pt[2], VIDEO_MEMORY + i * VIDEO_MEMORY_SIZE, VIDEO_MEMORY + i * VIDEO_MEMORY_SIZE, P | RW);
    }
    map_page_table_entry((page_table_t *)us_vid_mem_pt[0], VIDEO_MEMORY, VIDEO_MEMORY, P | RW | US);
    map_page_table_entry((page_table_t *)us_vid_mem_pt[1], VIDEO_MEMORY, VIDEO_MEMORY + 2 * VIDEO_MEMORY_SIZE, P | RW | US);
    map_page_table_entry((page_table_t *)us_vid_mem_pt[2], VIDEO_MEMORY, VIDEO_MEMORY + 3 * VIDEO_MEMORY_SIZE, P | RW | US);
 
    /* Setting up user program virutal to physical mapping from kernel's perspective */
    for (pid = 0; pid < MAX_NUM_PROCESSES; pid++) {
        uint32_t addr = PID_TO_PHYSICAL_ADDRESS(pid);
        map_page_directory_entry((page_directory_t *)kernel_pd, addr, addr, P | PS);
    }

    ENABLE_PAGING();
}

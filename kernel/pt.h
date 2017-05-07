#ifndef _PT_H
#define _PT_H

#include "pcb.h"
#include "stdbool.h"
#include "types.h"
#include "x86_desc.h"

/* General Bit Layout of a entries in a page table */
/*
 * Present (P) flag, bit 0
 *      - Indicates whether the page or page table being pointed to by the entry is currently loaded in physical memory.
 * Read/write (R/W) flag, bit 1
 *       - Specifies the read-write privileges for a page or group of pages
 * User/supervisor (U/S) flag, bit 2
 *       - Specifies the user-supervisor privileges for a page or group of pages. 0 means supervisor.
 * Page-level write-through (PWT) flag, bit 3
 *       - Controls the write-through or write-back caching policy of individual pages or page tables.
 * Page-level cache disable (PCD) flag, bit 4
 *       - Controls the caching of individual pages or page tables.
 * Accessed (A) flag, bit 5
 *       - Indicates whether a page or page table has been accessed (read from or written to) when set.
 * Dirty (D) flag, bit 6
 *       - Indicates whether a page has been written to when set.
 * Page size (PS) flag, bit 7 page-directory entries for 4-KByte pages
 *       - Determines the page size.
 * Page attribute table index (PAT) flag, bit 7 in for 4-KByte pages and bit 12 for 4-MByte pages
 *       - selects the memory type for the page
 * Global (G) flag, bit 8
 *       - Indicates a global page when set.
 * Reserved and available-to-software bits
 *       - Bits 9, 10, and 11 are available for use by software.
 */

#define NUM_PDE_ENTRIES 1024
#define NUM_PTE_ENTRIES 1024

// Where initial video memory for kernel is.
#define VIDEO_MEMORY 0xB8000

// The number of bits for the offset part of a 32 bit address.
#define NUM_4MB_OFFSET_BITS 22
#define NUM_4KB_OFFSET_BITS 12

/*Size of a 4KB page*/
#define VIDEO_MEMORY_SIZE (1 << NUM_4KB_OFFSET_BITS)/*in bytes*/

// Where in physical (and virtual) memory the kernel page is.
#define KERNEL_MEMORY 0x400000

// Where in virtual memory a program is loaded.
#define PROGRAM_VIRTUAL_ADDRESS 0x08048000

// Converts a zero index pid to it's starting physical addresss
// 4mb is the space of a program page
#define PID_TO_PHYSICAL_ADDRESS(pid) (KERNEL_MEMORY + (pid + 1) * 4 * MB)

#ifndef ASM
/* A Page-Directory Entry */
typedef uint32_t pde_t;
/* A Page Directory */
typedef struct page_directory {
    pde_t entries[NUM_PDE_ENTRIES];
} page_directory_t;

/*Page directory for the kernel itself*/
page_directory_t pd_kernel[1] __attribute__((aligned(sizeof(page_directory_t))));
/*An array of page directory. Index this array using pid*/
page_directory_t pds[MAX_NUM_PROCESSES] __attribute__((aligned(sizeof(page_directory_t))));

/* A Page-Table Entry */
typedef uint32_t pte_t;
/* A Page Table */
typedef struct page_table {
    pte_t entries[NUM_PTE_ENTRIES];
} page_table_t;

pte_t vid_mem_pt[NUM_PTE_ENTRIES] __attribute__((aligned(sizeof(page_table_t))));

/* Gets bit n from a bit map and return the result as a bool */
#define GET_BIT_N(bitmap, n) (!!((bitmap >> n) & 0x1))
/* Sets bit n in a bit map a with the provided flag (clamps to ensure a bool) */
#define SET_BIT_N(bitmap, n, flag) (bitmap = (bitmap & (~(1 << n))) | ((!!flag) << n))

/* Macros for getting a field of a page directory entry */
#define GET_P(entry) (GET_BIT_N(entry, 0))
#define GET_RW(entry) (GET_BIT_N(entry, 1))
#define GET_US(entry) (GET_BIT_N(entry, 2))
#define GET_PWT(entry) (GET_BIT_N(entry, 3))
#define GET_PCD(entry) (GET_BIT_N(entry, 4))
#define GET_A(entry) (GET_BIT_N(entry, 5))
#define GET_D(entry) (GET_BIT_N(entry, 6))
#define GET_PS(entry) (GET_BIT_N(entry, 7))
#define GET_G(entry) (GET_BIT_N(entry, 8))
/* Skiping Avail for now */
#define GET_4K_PAT(entry) (GET_BIT_N(entry, 7))
#define GET_4M_PAT(entry) (GET_BIT_N(entry, 12))
#define GET_PTE_ADDRESS(entry) (entry & ~0xFFF)
#define GET_4KB_PDE_ADDRESS(entry) (entry & ~0xFFF)
#define GET_4MB_PDE_ADDRESS(entry) (entry & ~0x3FFFFF)

/* Macros for setting a field in a page directory entry */
#define SET_P(entry, flag) (SET_BIT_N(entry, 0, flag))
#define SET_RW(entry, flag) (SET_BIT_N(entry, 1, flag))
#define SET_US(entry, flag) (SET_BIT_N(entry, 2, flag))
#define SET_PWT(entry, flag) (SET_BIT_N(entry, 3, flag))
#define SET_PCD(entry, flag) (SET_BIT_N(entry, 4, flag))
#define SET_A(entry, flag) (SET_BIT_N(entry, 5, flag))
#define SET_D(entry, flag) (SET_BIT_N(entry, 6, flag))
#define SET_PS(entry, flag) (SET_BIT_N(entry, 7, flag))
#define SET_G(entry, flag) (SET_BIT_N(entry, 8, flag))
/* Skiping Avail for now */
#define SET_4KB_PAT(entry) (SET_BIT_N(entry, 7))
#define SET_4MB_PAT(entry) (SET_BIT_N(entry, 12))
#define SET_PTE_ADDRESS(entry, address) (entry = (address & ~0xFFF) | (entry & 0xFFF))
#define SET_4KB_PDE_ADDRESS(entry, address) (entry = (address & ~0xFFF) | (entry & 0xFFF))
#define SET_4MB_PDE_ADDRESS(entry, address) (entry = (address & ~0x3FFFFF) | (entry & 0x3FFFFF))

/* Enum of virutal memory flags */
typedef enum virtual_memory_flags {
    P_BIT,
    RW_BIT,
    US_BIT,
    PWT_BIT,
    PCD_BIT,
    A_BIT,
    D_BIT,
    PS_BIT,
    G_BIT
} virtual_memory_flags_t;

/* Masks for virtual memory */
#define P (1 << P_BIT)
#define RW (1 << RW_BIT)
#define US (1 << US_BIT)
#define PWT (1 << PWT_BIT)
#define PCD (1 << PCD_BIT)
#define A (1 << A_BIT)
#define D (1 << D_BIT)
#define PS (1 << PS_BIT)
#define G (1 << G_BIT)
/* Maps a virtual address to a physical address in a page directory with the correct flags */
void map_page_directory_entry(page_directory_t *page_directory, uint32_t virtual_memory, uint32_t physical_memory, uint32_t flags);

/* Maps a virtual address to a physical address in a page table with the correct flags */
void map_page_table_entry(page_table_t *page_table, uint32_t virtual_memory, uint32_t physical_memory, uint32_t flags);

/*Check if a virtual address is in a user pages*/
uint8_t is_user(uint32_t virtual_memory);

// Inititialize the page directory and table.
void page_table_init();

/*
# Enables paging mode in x86 processor. Also enables 4 MB pages and loads
# register cr3 with the page table pointed to by 'pds'.
# Input: None
# Output: Enables paging bits in control registers.
# Return: None
*/
#define ENABLE_PAGING()                                                                                                                                                                                                                     \
    do {                                                                                                                                                                                                                                    \
        asm volatile(/* Load CR3 with the address of the page directory */                                                                                                                                                                  \
                     "movl  $pd_kernel, %%edx;"                                                                                                                                                                                             \
                     "movl  %%edx, %%cr3;" /* PSE flag in CR4 (bit 4) is set, */ /* both 4-MByte pages and page tables for 4-KByte pages can be accessed from the same page directory.*/                                                    \
                     "movl  %%cr4, %%edx;"                                                                                                                                                                                                  \
                     "orl   $0x00000010, %%edx;"                                                                                                                                                                                            \
                     "movl  %%edx, %%cr4;" /* Set the paging (PG) and protection (PE) bits of CR0. */ /* PE flag (bit 0)  in control register CR0—Enables protected mode */ /* PG flag (bit 31) in control register CR0—Enables paging */ \
                     "movl  %%cr0,  %%edx;"                                                                                                                                                                                                 \
                     "orl   $0x80000001, %%edx;"                                                                                                                                                                                            \
                     "movl  %%edx,  %%cr0;" ::                                                                                                                                                                                              \
                             : "memory", "edx");                                                                                                                                                                                            \
    } while (0)

/* Sets the CR3 Register */
#define SET_CR3(pd_ptr)                \
    do {                               \
        asm volatile(                  \
                "movl   %0, %%edx;"    \
                "movl   %%edx, %%cr3;" \
                :                      \
                : "g"(pd_ptr)          \
                : "memory", "edx");    \
    } while (0)

#endif /* ASM */
#endif /*_PT_H */

/* kernel.c - the C part of the kernel
 * vim:ts=4
 */

#include "debug.h"
#include "fs.h"
#include "idt.h"
#include "i8259.h"
#include "keyboard.h"
#include "lib.h"
#include "multiboot.h"
#include "pt.h"
#include "x86_desc.h"
#include "rtc.h"
#include "syscall.h"
#include "sys_execute.h"
#include "schedule.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

// Various checking of the magic bytes from the bootloader and the multiboot
// info struct also from the bootloader.
// Input: Magic bytes and pointer to multiboot info struct.
// Output: Prints out various values for debugging.
// Return: Returns 0 if everything is okay. Otherwise -1.
static int check_magic_and_mbi(unsigned long magic, multiboot_info_t *mbi)
{
    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return -1;
    }

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower,
               (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t *mod = (module_t *)mbi->mods_addr;
        init_fs((char *)mod->mod_start);
        while (mod_count < mbi->mods_count) {
            printf("Module %d loaded at address: 0x%#x\n", mod_count,
                   (unsigned int)mod->mod_start);
            printf("Module %d ends at address: 0x%#x\n", mod_count,
                   (unsigned int)mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char *)(mod->mod_start + i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return -1;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

        printf("elf_sec: num = %u, size = 0x%#x,"
               " addr = 0x%#x, shndx = 0x%#x\n",
               (unsigned)elf_sec->num, (unsigned)elf_sec->size,
               (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;

        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
               (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (memory_map_t *)mbi->mmap_addr;
             (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
             mmap = (memory_map_t *)((unsigned long)mmap + mmap->size +
                                     sizeof(mmap->size)))
            printf(" size = 0x%x,     base_addr = 0x%#x%#x\n"
                   "     type = 0x%x,  length    = 0x%#x%#x\n",
                   (unsigned)mmap->size, (unsigned)mmap->base_addr_high,
                   (unsigned)mmap->base_addr_low, (unsigned)mmap->type,
                   (unsigned)mmap->length_high, (unsigned)mmap->length_low);
    }

    return 0;
}

// Entrypoint to kernel. Checks if MAGIC is valid and print the Multiboot
// information structure pointed by ADDR. Also sets up devices, IRQ interrupts,
// exception handlers, GDT, LDT, page tables. Then infinite loops at end.
// Input: Magic bytes and pointer (|addr|) to multiboot info struct.
// Return: No value, but will return if something goes wrong.
void entry(unsigned long magic, unsigned long addr)
{
    multiboot_info_t *mbi;

    /* Clear the screen. */
    clear();

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *)addr;

    if (check_magic_and_mbi(magic, mbi)) {
        // Just stop if checks fail.
        return;
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0;
        the_ldt_desc.opsize = 1;
        the_ldt_desc.reserved = 0;
        the_ldt_desc.avail = 0;
        the_ldt_desc.present = 1;
        the_ldt_desc.dpl = 0x0;
        the_ldt_desc.sys = 0;
        the_ldt_desc.type = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity = 0;
        the_tss_desc.opsize = 0;
        the_tss_desc.reserved = 0;
        the_tss_desc.avail = 0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present = 1;
        the_tss_desc.dpl = 0x0;
        the_tss_desc.sys = 0;
        the_tss_desc.type = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }

    // pcb_init();
    idt_init();

    /* Init the PIC */
    i8259_init();
    enable_irq(2); // slave pic

    enable_irq(0); // system timer
    /*enable the keyboard*/
    keyboard_init();

    init_rtc();

    page_table_init();


    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    //printf("Enabling Interrupts\n");

    setup_syscalls();

    /* Execute the first program (`shell') ... */
    execute((uint8_t *)"shell");

    /* Spin (nicely, so we don't chew up cycles) */
    infinite_halt();
}

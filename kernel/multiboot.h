/* multiboot.h - Defines used in working with Multiboot-compliant
 * bootloaders (such as GRUB)
 * vim:ts=4
 */

#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#define MULTIBOOT_HEADER_FLAGS 0x00000003
/* The magic field should contain this. */
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
/* This should be in %eax. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#ifndef ASM

/* Types */
#include "types.h"

/* The Multiboot header. */
typedef struct multiboot_header {
    /* Must be MULTIBOOT_MAGIC - see above. */
    uint32_t magic;
    /* Feature flags. */
    uint32_t flags;
    /* The above fields plus this one must equal 0 mod 2^32. */
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
} multiboot_header_t;

/* The section header table for ELF. */
/*
 * These indicate where the section header table from an ELF kernel is
 * They correspond to the 'shdr_*' entries ('shdr_num', etc.)
 * in the Executable and Linkable Format (elf) specification in the program header.
 */
typedef struct elf_section_header_table {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info {
    /* Indicates the presence and validity of other fields in the Multiboot information structure. */
    uint32_t flags; /* (required) */
    /* Indicate the amount of lower and upper memory, respectively, in kilobytes. */
    uint32_t mem_lower; /* (present if flags[0] is set) */
    uint32_t mem_upper; /* (present if flags[0] is set) */
    /* Indicates which bios disk device the boot loader loaded the OS image from. */
    uint32_t boot_device; /* (present if flags[1] is set) */
    /*
     * Contains the physical address of the command line to be passed to the kernel.
     * The command line is a normal C-style zero-terminated string.
     */
    uint32_t cmdline; /* (present if flags[2] is set) */
    /*
     * the 'mods' fields indicate to the kernel what boot modules were loaded along with the kernel image,
     * and where they can be found.
     */
    /* Contains the number of modules loaded */
    uint32_t mods_count; /* (present if flags[3] is set) */
    /* Contains the physical address of the first module structure. */
    uint32_t mods_addr; /* (present if flags[3] is set) */
    /* Caution: Bits 4 & 5 are mutually exclusive. */
    elf_section_header_table_t elf_sec; /* (present if flags[4] or flags[5] is set) */
    /* Indicate the address and length of a buffer containing a memory map of the machine provided by the bios */
    uint32_t mmap_length; /* (present if flags[6] is set) */
    uint32_t mmap_addr;   /* (present if flags[6] is set) */
} multiboot_info_t;

typedef struct module {
    /* The first two fields contain the start and end addresses of the boot module itself */
    uint32_t mod_start;
    uint32_t mod_end;
    /* The 'string' field provides an arbitrary string to be associated with that particular boot module */
    uint32_t string;
    /* The 'reserved' field must be set to 0 by the boot loader and ignored by the operating system. */
    uint32_t reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map {
    /* 'size' is the size of the associated structure in bytes */
    uint32_t size;
    /* 'base_addr' is the starting address. */
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    /* 'length' is the size of the memory region in bytes. */
    uint32_t length_low;
    uint32_t length_high;
    /* 'type' is the variety of address range represented */
    uint32_t type;
} memory_map_t;

#endif /* ASM */

#endif /* _MULTIBOOT_H */

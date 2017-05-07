#ifndef _IDT_H
#define _IDT_H

#include "types.h"
#include "x86_desc.h"

/* Number of vectors in the interrupt descriptor table (IDT) */
#define NUM_VEC 256
// Number of IRQ handlers supported.
#define NUM_IRQ_HANDLER 15

/* Sets all the processor exceptions in the IDT */
// void idt_set_exceptions(void);

#ifndef ASM

/* An interrupt descriptor entry (goes into the IDT) */
typedef union idt_desc_t {
    uint32_t val;
    struct {
        uint16_t offset_15_00;
        uint16_t seg_selector;
        uint8_t reserved4;
        uint32_t reserved3 : 1;
        uint32_t reserved2 : 1;
        uint32_t reserved1 : 1;
        uint32_t size : 1;
        uint32_t reserved0 : 1;
        uint32_t dpl : 2;
        uint32_t present : 1;
        uint16_t offset_31_16;
    } __attribute__((packed));
} idt_desc_t;

/* The IDT itself */
extern idt_desc_t idt[NUM_VEC] __attribute__((aligned (16)));

/* The descriptor used to load the IDTR */
extern x86_desc_t idt_desc __attribute__((aligned (4)));

// The function pointer type of IRQ handlers.
typedef void (*irq_handler_func)();

/* Load the interrupt descriptor table (IDT).  This macro takes a 32-bit
 * address which points to a 6-byte structure.  The 6-byte structure
 * (defined as "struct x86_desc" above) contains a 2-byte size field
 * specifying the size of the IDT, and a 4-byte address field specifying
 * the base address of the IDT. */
#define lidt(desc)                \
    do {                          \
        asm volatile("lidt (%0)"  \
                     :            \
                     : "g"(desc)  \
                     : "memory"); \
    } while (0)

// Need to declare all x86 exception handlers here. They are defined in
// except.S. Note that most of them just call the appropriate default handler
// that is defined in idt.c.
void divide_by_zero_handler(void);
void debug_exception_handler(void);
void non_maskable_interrupt_handler(void);
void breakpoint_handler(void);
void overflow_handler(void);
void bound_exception_handler(void);
void invalid_opcode_handler(void);
void fpu_not_available_handler(void);
void double_fault_handler(void);
void coprocessor_segment_handler(void);
void invalid_TSS_handler(void);
void segment_not_present_handler(void);
void stack_exception_handler(void);
void general_protection_handler(void);
void page_fault_handler(void);
void floating_point_error_handler(void);
void alignment_check_handler(void);
void machine_check_handler(void);
void simd_floating_point_error_handler(void);

/* Inititializes the IDT */
void idt_init(void);

#endif /* ASM */
#endif /* _IDT_H */

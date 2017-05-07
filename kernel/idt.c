#include "idt.h"
#include "i8259.h"
#include "lib.h"
#include "keyboard.h"
#include "rtc.h"
#include "syscall.h"
#include "schedule.h"

/* The IDT itself */
idt_desc_t idt[NUM_VEC] __attribute__((aligned (16)));

x86_desc_t idt_desc = {
    .size = sizeof(idt) - 1,
    .addr = (uint32_t)idt
};

/* Sets runtime parameters for an IDT entry */
#define SET_IDT_ENTRY(str, handler)                                \
    do {                                                           \
        str.offset_31_16 = ((uint32_t)(handler)&0xFFFF0000) >> 16; \
        str.offset_15_00 = ((uint32_t)(handler)&0xFFFF);           \
    } while (0)

/* Sets an interrupt gate according to the intel manual */
/*
 * The Segment Selector inside the gate is set to the kernel code’s Segment Selector.
 * The handler field is set to addr, which is the address of the interrupt handler.
 * The DPL field is set to 0.
 */
#define SET_INTERRUPT_GATE(str, handler) \
    do {                                 \
        SET_IDT_ENTRY(str, handler);     \
        str.seg_selector = KERNEL_CS;    \
        str.reserved2 = 1;               \
        str.reserved1 = 1;               \
        str.size = 1;                    \
        str.dpl = 0;                     \
        str.present = 1;                 \
    } while (0)

// Saves all registers onto the stack. Designed for inline assembly usage.
#define SAVE_REG_X86 \
    "pushl  %eax;"   \
    "pushl  %ebx;"   \
    "pushl  %ecx;"   \
    "pushl  %edx;"   \
    "pushl  %esi;"   \
    "pushl  %edi;"   \
    "pushl  %ebp;"   \
    "pushl  %ds;"
// C version of SAVE_REG_X86.
#define SAVE_REG()                          \
    do {                                    \
        asm volatile(                       \
                "pushl  %%eax;"             \
                "pushl  %%ebx;"             \
                "pushl  %%ecx;"             \
                "pushl  %%edx;"             \
                "pushl  %%esi;"             \
                "pushl  %%edi;"             \
                "pushl  %%ebp;"             \
                "pushl  %%ds;" ::           \
                        : "memory", "esp"); \
    } while (0)

// Size that all the saved registers take up on stack.
// 7 registers, 4 bytes each.
#define SAVE_REG_SIZE_STR "8*4"

// Restores all registers from the stack as stored by SAVE_REG()
#define RESTORE_REG_X86 \
    "popl   %ds;"       \
    "popl   %ebp;"      \
    "popl   %edi;"      \
    "popl   %esi;"      \
    "popl   %edx;"      \
    "popl   %ecx;"      \
    "popl   %ebx;"      \
    "popl   %eax;"

// C version of RESTORE_REG_X86.
#define RESTORE_REG()                                         \
    do {                                                      \
        asm volatile(                                         \
                "popl   %%ds;"                                \
                "popl   %%ebp;"                               \
                "popl   %%edi;"                               \
                "popl   %%esi;"                               \
                "popl   %%edx;"                               \
                "popl   %%ecx;"                               \
                "popl   %%ebx;"                               \
                "popl   %%eax;" ::                            \
                        : "memory", "eax", "ebx", "ecx",      \
                          "edx", "esi", "edi", "ebp", "esp"); \
    } while (0)

// Defines a wrapper to an irq_handler_func named |handler_name| that will be
// used by SET_IRQ_HANDLER().
#define DEFINE_IRQ_HANDLER_WRAPPER(irq_num, handler_name) \
    void handler_name##_wrapper();                        \
    asm(#handler_name                                     \
        "_wrapper:"                                       \
        "pushl $" #irq_num ";"                            \
        "jmp   irq_handler_wrapper;")

// Sets the IDT entry to point to an IRQ handler wrapper that wraps the
// |handler_name| function. |handler_name| should be the name of the C function
// IRQ handler of type irq_handler_func.
#define SET_IRQ_HANDLER(irq_num, handler_name)                                  \
    do {                                                                        \
        irq_handler_table[irq_num] = handler_name;                              \
        SET_INTERRUPT_GATE(idt[ICW2_MASTER + irq_num], handler_name##_wrapper); \
    } while (0)

/* Sets a system gate according to the intel manual */
/*
 * The Segment Selector inside the gate is set to the kernel code’s Segment Selector.
 * The Offset field is set to addr, which is the address of the exception handler.
 * The DPL field is set to 3.
 */
#define SET_SYSTEM_GATE(str, handler)     \
    do {                                  \
        SET_INTERRUPT_GATE(str, handler); \
        str.reserved3 = 1;                \
        str.dpl = 3;                      \
    } while (0)

/* Sets a trap gate according to the intel manual */
/*
 * Similar to the previous function, except the DPL field is set to 0.
 */
#define SET_TRAP_GATE(str, handler)    \
    do {                               \
        SET_SYSTEM_GATE(str, handler); \
        str.dpl = 0;                   \
    } while (0)


// Stores the C function handlers corresponding to each IRQ. The index in the
// table is the IRQ number of the handler.
irq_handler_func irq_handler_table[NUM_IRQ_HANDLER];


// All IRQ interrupts first call this wrapper which will call the appropriate C
// function IRQ handler. This way the C function does not need to use iret or
// save all registers.
// Input: IRQ number should be on top of stack.
// Output: Runs C function handler corresponding to IRQ num.
asm("irq_handler_wrapper:"
           SAVE_REG_X86
    "      movl  $" STRINGIFY2(KERNEL_DS) ", %eax;"
    "      movl  %eax, %ds;"

    "      pushl   " SAVE_REG_SIZE_STR "(%esp);" // Need to pass IRQ num to send_eoi().
    "      call    send_eoi;"
    "      addl    $4, %esp;" // Pop send_eoi() argument off.

    "      pushl   " SAVE_REG_SIZE_STR "(%esp);"
    "      call    disable_irq;"
    "      addl    $4, %esp;"

    "      sti;"
    "      movl    " SAVE_REG_SIZE_STR "(%esp), %eax;"
    "      call    *irq_handler_table(, %eax, 4);"
    "      cli;"

    "      pushl   " SAVE_REG_SIZE_STR "(%esp);"
    "      call    enable_irq;"
    "      addl    $4, %esp;"

           RESTORE_REG_X86
    "      addl    $4, %esp;" // Need to restore stack. IRQ num is on stack.
    "      iret;");

/*
Processor Exceptions

Exception Types:

* Fault - the return address points to the instruction that caused the exception. The exception handler may fix the problem and then restart the program, making it look like nothing has happened.
* Trap - the return address points to the instruction after the one that has just completed.
* Abort - the return address is not always reliably supplied. A program which causes an abort is never meant to be continued.

Number  Description                                         Type
----------------------------------------------------------------------
0       Divide-By-Zero                                      fault
1       Debug Exception	                                    trap or fault
2       Non-Maskable Interrupt (NMI)	                    trap
3       Breakpoint (INT 3)	                                trap
4       Overflow (INTO with EFlags[OF] set)                 trap
5       Bound Exception (BOUND on out-of-bounds access)     trap
6       Invalid Opcode	                                    trap
7       FPU Not Available	                                trap
8*      Double Fault	                                    abort
9       Coprocessor Segment Overrun	                        abort
10*     Invalid TSS	                                        fault
11*     Segment Not Present	                                fault
12*     Stack Exception	                                    fault
13*     General Protection	                                fault or trap
14*     Page fault	                                        fault
15      Reserved
16      Floating-Point Error                                fault
17      Alignment Check                                     fault
18      Machine Check                                       abort
19-31	Reserved By Intel
32-255	Available for Software and Hardware Interrupts
*These exceptions have an associated error code.
*/

// Note that the following BSOD handlers simply just blue screen. The actual
// handler is defined in except.S (which for unimplemented handlers, simply
// calls the BSOD handler here).

// C function exception handler of type irq_handler_func that just blue
// screens.
void divide_by_zero_bsod_handler(void)
{
    BSOD(" Division By Zero Error ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void debug_exception_bsod_handler(void)
{
    BSOD(" Debug Exception ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void non_maskable_interrupt_bsod_handler(void)
{
    BSOD(" Non-Maskable Interrupt ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void breakpoint_bsod_handler(void)
{
    BSOD(" Breakpoint ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void overflow_bsod_handler(void)
{
    BSOD(" Overflow ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void bound_exception_bsod_handler(void)
{
    BSOD(" Bound Exception ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void invalid_opcode_bsod_handler(void)
{
    BSOD(" Invalid Opcode ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void fpu_not_available_bsod_handler(void)
{
    BSOD(" FPU Not Available ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void double_fault_bsod_handler(void)
{
    BSOD(" Double Fault ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void coprocessor_segment_bsod_handler(void)
{
    BSOD(" Coprocessor Segment Overrun ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void invalid_TSS_bsod_handler(void)
{
    BSOD(" Invalid TSS ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void segment_not_present_bsod_handler(void)
{
    BSOD(" Segment Not Present ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void stack_exception_bsod_handler(void)
{
    BSOD(" Stack Exception ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void general_protection_bsod_handler(void)
{
    BSOD(" General Protection ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void page_fault_bsod_handler(void)
{
    BSOD(" Page Fault ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void floating_point_error_bsod_handler(void)
{
    BSOD(" Floating-point Error ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void alignment_check_bsod_handler(void)
{
    BSOD(" Alignment Check ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
// processor).
void machine_check_bsod_handler(void)
{
    BSOD(" Machine Check ");
}

// C function exception handler of type irq_handler_func that just blue
// screens.
void simd_floating_point_error_bsod_handler(void)
{
    BSOD(" SIMD Floating Point Error ");
}



/*
 * idt_set_exception
 *   DESCRIPTION:  Set all the exception handlers in the IDT.
 *   INPUTS: void
 *   OUTPUTS: void
 *   RETURN VALUE: none
 */
void idt_set_exceptions(void)
{
    // Setting these all to interrupt gates because of a Piazza post saying
    // nothing else (interrupts) should happen after an exception.
    SET_INTERRUPT_GATE(idt[0], divide_by_zero_handler);
    SET_INTERRUPT_GATE(idt[1], debug_exception_handler);
    SET_INTERRUPT_GATE(idt[2], non_maskable_interrupt_handler);
    SET_INTERRUPT_GATE(idt[3], breakpoint_handler);
    SET_INTERRUPT_GATE(idt[4], overflow_handler);
    SET_INTERRUPT_GATE(idt[5], bound_exception_handler);
    SET_INTERRUPT_GATE(idt[6], invalid_opcode_handler);
    SET_INTERRUPT_GATE(idt[7], fpu_not_available_handler);
    SET_INTERRUPT_GATE(idt[8], double_fault_handler);
    SET_INTERRUPT_GATE(idt[9], coprocessor_segment_handler);
    SET_INTERRUPT_GATE(idt[10], invalid_TSS_handler);
    SET_INTERRUPT_GATE(idt[11], segment_not_present_handler);
    SET_INTERRUPT_GATE(idt[12], stack_exception_handler);
    SET_INTERRUPT_GATE(idt[13], general_protection_handler);
    SET_INTERRUPT_GATE(idt[14], page_fault_handler);
    // Skip 15 because reserved by Intel for unknown purposes.
    SET_INTERRUPT_GATE(idt[16], floating_point_error_handler);
    SET_INTERRUPT_GATE(idt[17], alignment_check_handler);
    SET_INTERRUPT_GATE(idt[18], machine_check_handler);
    SET_INTERRUPT_GATE(idt[19], simd_floating_point_error_handler);
}

/* Hardware Interrupts */
/*
 * Master PIC
 *      IRQ 0 – System Timer (cannot be changed)
 *      IRQ 1 – Keyboard Controller (cannot be changed)
 *      IRQ 2 – Cascaded Signals From IRQs 8–15 (any devices configured to use IRQ 2 will actually be using IRQ 9)
 *      IRQ 3 – Serial Port Controller For Serial Port 2 (shared with serial port 4, if present)
 *      IRQ 4 – Serial Port Controller For Serial Port 1 (shared with serial port 3, if present)
 *      IRQ 5 – Parallel Port 2 And 3  Or  Sound card
 *      IRQ 6 – Floppy Disk Controller
 *      IRQ 7 – Parallel Port 1. It is used for printers or for any parallel port if a printer is not present. It can also be potentially be shared with a secondary sound card with careful management of the port.
 *
 * Slave PIC
 *      IRQ 8 – Real-Time Clock (RTC)
 *      IRQ 9 – Advanced Configuration And Power Interface (ACPI) System Control Interrupt On Intel Chipsets.[1] Other chipset manufacturers might use another interrupt for this purpose, or make it available for the use of peripherals (any devices configured to use IRQ 2 will actually be using IRQ 9)
 *      IRQ 10 – The Interrupt Is Left Open For The Use Of Peripherals (open interrupt/available, SCSI or NIC)
 *      IRQ 11 – The Interrupt Is Left Open For The Use Of Peripherals (open interrupt/available, SCSI or NIC)
 *      IRQ 12 – Mouse On PS/2 Connector
 *      IRQ 13 – CPU Co-Processor  Or  Integrated Floating Point Unit Or Inter-Processor Interrupt (use depends on OS)
 *      IRQ 14 – Primary ATA Channel (ATA interface usually serves hard disk drives and CD drives)
 *      IRQ 15 – Secondary ATA Channel
 */

// IRQ 0
// C function interrupt handler of type irq_handler_func that will be called to
// handle a system timer interrupt.
void system_timer_handler(void)
{
    schedule();
}

// IRQ 1
// C function interrupt handler of type irq_handler_func that will be called to
// handle a keyboard interrupt.
//
// If the combination of keys only contain function keys, nothing will be
// performed except for setting function key flags when keys are pressed, and
// clearing the flags when keys are released.
void keyboard_controller_handler(void)
{
    keyboard_driver();
}

// No handler for IRQ2 - cascaded signals from IRQs 8 - 15 from slave.

// IRQ 3
// C function interrupt handler of type irq_handler_func that will be called to
// handle a serial port 2 interrupt.
void serial_port_2_handler(void)
{
    EXCEPTION(" Serial Port 2 ");
}

// IRQ 4
// C function interrupt handler of type irq_handler_func that will be called to
// handle a system timer interrupt.
void serial_port_1_handler(void)
{
    EXCEPTION(" Serial Port 1 ");
}

// IRQ 5
// C function interrupt handler of type irq_handler_func that will be called to
// handle parallel port 2 & 3 or the sound card interrupts.
void parallel_port_2_3_or_sound_handler(void)
{
    EXCEPTION(" Parallel Port 2 / 3 or Sound Card ");
}

// IRQ 6
// C function interrupt handler of type irq_handler_func that will be called to
// handle floppy disk interrupts.
void floppy_disk_controller_handler(void)
{
    EXCEPTION(" Floppy Disk ");
}

// IRQ 7
// C function interrupt handler of type irq_handler_func that will be called to
// handle parallel port interrupts.
void parallel_port_1_handler(void)
{
    EXCEPTION(" Parallel Port 1 ");
}

// IRQ 8
// C function interrupt handler of type irq_handler_func that will be called to
// handle the real time clock interrupts.
void real_time_clock_handler(void)
{
    rtc_interrupt_handler();
}

// IRQ 9
// C function interrupt handler of type irq_handler_func that will be called to
// handle acpi controller interrupts.
void acpi_controller_handler(void)
{
    EXCEPTION(" ACPI ");
}

// IRQ 10
// C function interrupt handler of type irq_handler_func that will be called to
// handle peripheral 1's interrupts.
void peripherals_1_handler(void)
{
    EXCEPTION(" Peripherals 1 ");
}

// IRQ 11
// C function interrupt handler of type irq_handler_func that will be called to
// handle peripheral 2's interrupts.
void peripherals_2_handler(void)
{
    EXCEPTION(" Peripherals 2 ");
}

// IRQ 12
// C function interrupt handler of type irq_handler_func that will be called to
// handle the mouse/ps2 interrupts.
void mouse_ps2_handler(void)
{
    EXCEPTION(" Mouse On PS/2 ");
}

// IRQ 13
// C function interrupt handler of type irq_handler_func that will be called to
// handle coprocessor, FPU unit, or inter-processor interrupts.
void processor_handler(void)
{
    EXCEPTION(" CPU Co-Processor  Or  Integrated Floating Point Unit  Or  Inter-Processor Interrupt (Use Depends On OS) ");
}

// IRQ 14
// C function interrupt handler of type irq_handler_func that will be called to
// handle the primary ATA channel interrupts.
void primary_ata_channel_handler(void)
{
    EXCEPTION(" Primary ATA Channel ");
}

// Whenever setup IRQ handler in idt_set_hardware_interrupts(), need to also
// define the wrapper here. Needs to be out here because C doesn't support
// nested function declaration.
DEFINE_IRQ_HANDLER_WRAPPER(0, system_timer_handler);
DEFINE_IRQ_HANDLER_WRAPPER(1, keyboard_controller_handler);
DEFINE_IRQ_HANDLER_WRAPPER(3, serial_port_2_handler);
DEFINE_IRQ_HANDLER_WRAPPER(4, serial_port_1_handler);
DEFINE_IRQ_HANDLER_WRAPPER(5, parallel_port_2_3_or_sound_handler);
DEFINE_IRQ_HANDLER_WRAPPER(6, floppy_disk_controller_handler);
DEFINE_IRQ_HANDLER_WRAPPER(7, parallel_port_1_handler);
DEFINE_IRQ_HANDLER_WRAPPER(8, real_time_clock_handler);
DEFINE_IRQ_HANDLER_WRAPPER(9, acpi_controller_handler);
DEFINE_IRQ_HANDLER_WRAPPER(10, peripherals_1_handler);
DEFINE_IRQ_HANDLER_WRAPPER(11, peripherals_2_handler);
DEFINE_IRQ_HANDLER_WRAPPER(12, mouse_ps2_handler);
DEFINE_IRQ_HANDLER_WRAPPER(13, processor_handler);
DEFINE_IRQ_HANDLER_WRAPPER(14, primary_ata_channel_handler);

// Sets up handlers for all possible hardware interrupts in the IDT. Note that
// although it isn't necessary to set them all up, it makes debugging easier if
// the wrong IDT entry is used for an IRQ interrupt.
void idt_set_hardware_interrupts(void)
{
    SET_IRQ_HANDLER(0, system_timer_handler);
    SET_IRQ_HANDLER(1, keyboard_controller_handler);
    /* Skipping 2 since that is IRQ 2 and the slave is connected there */
    SET_IRQ_HANDLER(3, serial_port_2_handler);
    SET_IRQ_HANDLER(4, serial_port_1_handler);
    SET_IRQ_HANDLER(5, parallel_port_2_3_or_sound_handler);
    SET_IRQ_HANDLER(6, floppy_disk_controller_handler);
    SET_IRQ_HANDLER(7, parallel_port_1_handler);
    SET_IRQ_HANDLER(8, real_time_clock_handler);
    SET_IRQ_HANDLER(9, acpi_controller_handler);
    SET_IRQ_HANDLER(10, peripherals_1_handler);
    SET_IRQ_HANDLER(11, peripherals_2_handler);
    SET_IRQ_HANDLER(12, mouse_ps2_handler);
    SET_IRQ_HANDLER(13, processor_handler);
    SET_IRQ_HANDLER(14, primary_ata_channel_handler);
}

// Sets up all interrupts/exceptions in IDT table.
void idt_set_interrupts(void)
{
    idt_set_exceptions();
    idt_set_hardware_interrupts();

    // Support system call through INT 0x80.
    SET_SYSTEM_GATE(idt[0x80], system_call_handler);
}

void idt_init(void)
{
    lidt(idt_desc);
    idt_set_interrupts();
}

/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4
 */

#include "i8259.h"
#include "lib.h"

/* Modified from the acutal i8259 in the linux kernel */

/*
 * Master PIC
 *      IRQ 0 – system timer (cannot be changed)
 *      IRQ 1 – keyboard controller (cannot be changed)
 *      IRQ 2 – cascaded signals from IRQs 8–15 (any devices configured to use IRQ 2 will actually be using IRQ 9)
 *      IRQ 3 – serial port controller for serial port 2 (shared with serial port 4, if present)
 *      IRQ 4 – serial port controller for serial port 1 (shared with serial port 3, if present)
 *      IRQ 5 – parallel port 2 and 3  or  sound card
 *      IRQ 6 – floppy disk controller
 *      IRQ 7 – parallel port 1. It is used for printers or for any parallel port if a printer is not present. It can also be potentially be shared with a secondary sound card with careful management of the port.
 *
 * Slave PIC[edit]
 *      IRQ 8 – real-time clock (RTC)
 *      IRQ 9 – Advanced Configuration and Power Interface (ACPI) system control interrupt on Intel chipsets.[1] Other chipset manufacturers might use another interrupt for this purpose, or make it available for the use of peripherals (any devices configured to use IRQ 2 will actually be using IRQ 9)
 *      IRQ 10 – The Interrupt is left open for the use of peripherals (open interrupt/available, SCSI or NIC)
 *      IRQ 11 – The Interrupt is left open for the use of peripherals (open interrupt/available, SCSI or NIC)
 *      IRQ 12 – mouse on PS/2 connector
 *      IRQ 13 – CPU co-processor  or  integrated floating point unit  or  inter-processor interrupt (use depends on OS)
 *      IRQ 14 – primary ATA channel (ATA interface usually serves hard disk drives and CD drives)
 *      IRQ 15 – secondary ATA channel
 */

/*
 * The PIC has an internal register called the IMR, or the Interrupt Mask Register.
 * It is 8 bits wide. This register is a bitmap of the request lines going into the PIC.
 * When a bit is set, the PIC ignores the request and continues normal operation.
 * Note that setting the mask on a higher request line will not affect a lower line.
 * Masking IRQ2 will cause the Slave PIC to stop raising IRQs.
 */

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7 */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void)
{
    /* Not using spinlock, since this is a unicore machine */
    unsigned long flags;
    cli_and_save(flags);

    outb(0xFF, MASTER_8259_DATA); /* Mask all of Master */
    outb(0xFF, SLAVE_8259_DATA);  /* Mask all of Slave */

    /* Not adding pauses (outb_p, since only testing one hardware) */

    /* Init the Master */
    outb(ICW1, MASTER_8259_PORT);        /* ICW1: select Master init */
    outb(ICW2_MASTER, MASTER_8259_DATA); /* ICW2: Master IR0-7 mapped to 0x20-0x27 */
    outb(ICW3_MASTER, MASTER_8259_DATA); /* Master has a slave on IR2 */
    outb(ICW4, MASTER_8259_DATA);        /* Master expected normal EOI */

    /* Init the Slave */
    outb(ICW1, SLAVE_8259_PORT);       /* ICW1: select slave init */
    outb(ICW2_SLAVE, SLAVE_8259_DATA); /* ICW2: Slave IR0-7 mapped to 0x28-0x2f */
    outb(ICW3_SLAVE, SLAVE_8259_DATA); /* Slave is a slave on master's IR2 */
    outb(ICW4, SLAVE_8259_DATA);       /* (slave's support for AEOI in flat mode is to be investigated) */

    outb(master_mask, MASTER_8259_DATA); /* restore master irq mask */
    outb(slave_mask, SLAVE_8259_DATA);   /* restore slave irq mask */

    restore_flags(flags);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num)
{
    unsigned long flags;

    cli_and_save(flags);

    if (irq_num < 8) {
        master_mask &= ~(1 << irq_num);
        outb(master_mask, MASTER_8259_DATA);
    } else {
        irq_num -= 8;
        slave_mask &= ~(1 << irq_num);
        outb(slave_mask, SLAVE_8259_DATA);
    }

    restore_flags(flags);
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num)
{
    unsigned int mask;
    unsigned long flags;

    mask = 1 << irq_num;
    cli_and_save(flags);

    if (irq_num < 8) {
        master_mask |= (1 << irq_num);
        outb(master_mask, MASTER_8259_DATA);
    } else {
        irq_num -= 8;
        slave_mask |= (1 << irq_num);
        outb(slave_mask, SLAVE_8259_DATA);
    }

    restore_flags(flags);
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num)
{
    unsigned long flags;
    cli_and_save(flags);

    /*
     * If the IRQ came from the Master PIC, it is sufficient to issue this command only to the Master PIC;
     * however if the IRQ came from the Slave PIC, it is necessary to issue the command to both PIC chips.
     */
    if (irq_num >= 8) {
        outb(EOI | (irq_num - 8), SLAVE_8259_PORT); /* Sending EOI to slave */
        outb(EOI | ICW3_SLAVE, MASTER_8259_PORT);   /* 'Specific EOI' to master-IRQ2 */
    } else {
        outb(EOI | irq_num, MASTER_8259_PORT); /* Sending EOI to master */
    }

    restore_flags(flags);
}

/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4
 */

#ifndef _I8259_H
#define _I8259_H

#include "types.h"

/* Ports that each PIC sits on */
#define MASTER_8259_PORT 0x20
#define MASTER_8259_DATA (MASTER_8259_PORT + 1)
#define SLAVE_8259_PORT  0xA0
#define SLAVE_8259_DATA  (SLAVE_8259_PORT + 1)

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
// Start init and expect ICW4.
#define ICW1            0x11
// Vector number of first vector for this PIC in IDT.
#define ICW2_MASTER     (MASTER_8259_PORT + 0)
#define ICW2_SLAVE      (ICW2_MASTER + 8)
// Tell master which IRQ slave is connected.
#define ICW3_MASTER     0x04
// Tells slave its identifying number.
#define ICW3_SLAVE      0x02
// Using 8086 system.
#define ICW4            0x01

/* Specific end-of-interrupt byte. This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI             0x60

/* Externally-visible functions */

/* Initialize both PICs */
void i8259_init(void);
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);

#endif /* _I8259_H */

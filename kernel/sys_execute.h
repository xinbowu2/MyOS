#ifndef _SYS_EXECUTE_H
#define _SYS_EXECUTE_H

#include "types.h"
int32_t execute(const uint8_t *command);

/* Store a register value into a C variable */
#define STORE_REGISTER_VALUE(register, int_addr)                  \
    do {                                                          \
        asm volatile(                                             \
                "movl   %%" #register ", (%0);" ::"g"(int_addr)); \
    } while (0)

/* Pushes a constant onto the stack */
#define pushl(value)                                \
    do {                                            \
        asm volatile("pushl $" STRINGIFY(value) ";" \
                     :                              \
                     :                              \
                     : "memory");                   \
    } while (0)

/* Pushes EFLAGS onto stack */
#define pushfl()                  \
    do {                          \
        asm volatile("pushfl;"    \
                     :            \
                     :            \
                     : "memory"); \
    } while (0)

#endif // _SYS_EXECUTE_H

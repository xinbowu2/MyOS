#ifndef _SYS_HALT_H
#define _SYS_HALT_H

#define PROCESS_RETURN_SUCCESS 0
#define PROCESS_RETURN_EXCEPT 256

#ifndef ASM

#include "fs.h"
#include "pcb.h"
#include "pt.h"

/*
 * The halt system call terminates a process, returning the specied value to its parent process.
 * The system call handler itself is responsible for expanding the 8-bit argument from BL into the
 * 32-bit return value to the parent program's execute system call.
 * Be careful not to return all 32 bits from EBX.
 * This call should never return to caller.
 */
int32_t halt(uint8_t status);

/* Moves a value into the register */
#define MOVE_REGISTER_VALUE(register, int_addr)              \
    do {                                                     \
        asm volatile(                                        \
                "movl  %0, %%" #register ";" ::"g"(int_addr) \
                : #register);                                \
    } while (0)

#endif // ASM

#endif // _SYS_HALT_H

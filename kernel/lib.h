/* lib.h - Defines for useful library functions
 * vim:ts=4
 */

#ifndef _LIB_H
#define _LIB_H
#define SCREEN_WIDTH 80
#define NUM_ROWS 25

#include "types.h"
// #include "schedule.h"

#define MAX_NUM_TERMINALS 3

/* Makes a string out of a macro */
#define STRINGIFY(x) #x

/* Just calls STRINGIFY */
#define STRINGIFY2(x) STRINGIFY(x)
// Simply calls bsod(). Still around for backwards compatibility.
#define BSOD(message)    \
    do {                 \
        bsod(message);   \
    } while (0)

// Prints out to screen a message. Can also clear screen first for readability.
// Input: The string to printout.
// Output: Prints the message to the screen.
#define EXCEPTION(message)          \
    do {                            \
        /*clear();*/                \
        printf("%s\n", message);    \
    } while (0)

extern int char_count;
extern int8_t *prompt;
extern int new_line_checklist[MAX_NUM_TERMINALS][NUM_ROWS];



int32_t printf(const int8_t *format, ...);
void putc(uint8_t c);
int32_t puts(int8_t *s);
int8_t *itoa(uint32_t value, int8_t *buf, int32_t radix);
int8_t *strrev(int8_t *s);
uint32_t strlen(const int8_t *s);
void clear(void);
void clear_row(int row_index);
void blue_screen(void);
void bsod(const char *message);
// Implemented in except.S.
void infinite_halt();

void *memset(void *s, int32_t c, uint32_t n);
void *memset_word(void *s, int32_t c, uint32_t n);
void *memset_dword(void *s, int32_t c, uint32_t n);
void *memcpy(void *dest, const void *src, uint32_t n);
void *memmove(void *dest, const void *src, uint32_t n);
int32_t strncmp(const int8_t *s1, const int8_t *s2, uint32_t n);
int8_t *strcpy(int8_t *dest, const int8_t *src);
int8_t *strncpy(int8_t *dest, const int8_t *src, uint32_t n);

/* Userspace address-check functions */
int32_t bad_userspace_addr(const void *addr, int32_t len);
int32_t safe_strncpy(int8_t *dest, const int8_t *src, int32_t n);

void debug_interrupts(void);

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(port)
{
    uint32_t val;
    asm volatile(
            "xorl %0, %0\n \
             inb   (%w1), %b0"
            : "=a"(val)
            : "d"(port)
            : "memory");
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(port)
{
    uint32_t val;
    asm volatile(
            "xorl %0, %0\n   \
             inw   (%w1), %w0"
            : "=a"(val)
            : "d"(port)
            : "memory");
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(port)
{
    uint32_t val;
    asm volatile("inl   (%w1), %0"
                 : "=a"(val)
                 : "d"(port)
                 : "memory");
    return val;
}

/* Writes a byte to a port */
#define outb(data, port)                    \
    do {                                    \
        asm volatile("outb  %b1, (%w0)"     \
                     :                      \
                     : "d"(port), "a"(data) \
                     : "memory", "cc");     \
    } while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                    \
    do {                                    \
        asm volatile("outw  %w1, (%w0)"     \
                     :                      \
                     : "d"(port), "a"(data) \
                     : "memory", "cc");     \
    } while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                    \
    do {                                    \
        asm volatile("outl  %l1, (%w0)"     \
                     :                      \
                     : "d"(port), "a"(data) \
                     : "memory", "cc");     \
    } while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
    do {                                \
        asm volatile("cli"              \
                     :                  \
                     :                  \
                     : "memory", "cc"); \
    } while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)    \
    do {                       \
        asm volatile(          \
            "pushfl          \n\
             popl %0         \n\
             cli"              \
            : "=r"(flags)      \
            :                  \
            : "memory", "cc"); \
    } while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
    do {                                \
        asm volatile("sti"              \
                     :                  \
                     :                  \
                     : "memory", "cc"); \
    } while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most
 * often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)    \
    do {                        \
        asm volatile(           \
            "pushl %0         \n\
             popfl"             \
             :                  \
             : "r"(flags)       \
             : "memory", "cc"); \
    } while (0)

// Format string for printing all the registers.
extern const char *const ALL_REG_FMT_STR;

// Prints all registers for debugging purposes.
// Output: Prints the value in every register.
#define print_all_regs()                \
    do {                                \
        asm volatile(                   \
            "pushl  %%esp;"             \
            "pushl  %%ebp;"             \
            "pushl  %%edi;"             \
            "pushl  %%esi;"             \
            "pushl  %%edx;"             \
            "pushl  %%ecx;"             \
            "pushl  %%ebx;"             \
            "pushl  %%eax;"             \
            "pushl  ALL_REG_FMT_STR;"   \
            "call   printf;"            \
            "addl   $4, %%esp;"         \
            "popl   %%eax;"             \
            "popl   %%ebx;"             \
            "popl   %%ecx;"             \
            "popl   %%edx;"             \
            "popl   %%esi;"             \
            "popl   %%edi;"             \
            "popl   %%ebp;"             \
            "popl   %%esp;":);          \
    } while (0)

#endif /* _LIB_H */

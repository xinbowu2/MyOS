/*rtc.c - functions for interacting with the RTC*/

#include "rtc.h"
#include "i8259.h"
#include "lib.h"

/*port select*/
#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT 0x71

/*Indexes of registers*/
#define RTC_REGISTER_A 0x0A
#define RTC_REGISTER_B 0x0B
#define RTC_REGISTER_C 0x0C
#define RTC_REGISTER_D 0x0D

/*Disable Non-maskable Interrupt setting on upper four bits on the data port*/
#define RTC_DISABLE_NMI 0x80

/*IRQ Interrupt Line*/
#define RTC_IRQ_NUM 8

/*Interrupt Flag States*/
#define SET 1
#define CLEAR 0

/*check if the input number is a power of 2*/
#define CHECK_POWER2(num) (num != 0 && (num & (num - 1)) == 0)

/*a flag is cleared, if a rtc interrupt is handled*/
volatile char rtc_flag = SET;

/*
 * init_rtc
 *   DESCRIPTION: Initialize the RTC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Turning on IRQ line 8 for the RTC
 */
void init_rtc(void)
{
    // Turning on IRQ 8 for RTC
    char prev;
    outb(RTC_DISABLE_NMI | RTC_REGISTER_B, RTC_INDEX_PORT); // select register B, and disable NMI
    prev = inb(RTC_DATA_PORT);                              // read the current value of register B
    outb(RTC_DISABLE_NMI | RTC_REGISTER_B, RTC_INDEX_PORT); // set the index again (a read will reset the index to register D)
    outb(prev | 0x40, RTC_DATA_PORT);                       // write the previous value ORed with 0x40. This turns on bit 6 of register B

    rtc_set_freq(DEFAULT_FREQ); // set the frequency to 2Hz by default
    enable_irq(RTC_IRQ_NUM);    //enable irq 8 line for RTC
}

/*
 * rtc_open
 *   DESCRIPTION: open a RTC file
 *   INPUTS: filename -- filename of the RTC driver file
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for success
 *				   -1, if the file does not exist
 *   SIDE EFFECTS: none
 */
int32_t
rtc_open(const uint8_t *file_name)
{
    (void)file_name;

    //since we donot have related system calls, we do nothing.

    rtc_set_freq(DEFAULT_FREQ); // set the frequency to 2Hz by default
    return 0;
}

/*
 * rtc_read
 *   DESCRIPTION: read from the RTC
 *   INPUTS: fd -- access to the RTC file
 *			 buf -- data buffer
 *			 nbytes -- the number of bytes of the
 *   OUTPUTS: none
 *   RETURN VALUE: 0, if an interrupt has occurred
 *   SIDE EFFECTS: none
 */
int32_t
rtc_read(int32_t fd, void *buf, int32_t nbytes)
{
    rtc_flag = SET;    //reset the flag
    while (rtc_flag) { /*printf("rtc reading\n");*/
    };                 //wait until the interrupt handler clear the flag

    return 0;
}

/*
 * rtc_write
 *   DESCRIPTION: write to the RTC
 *   INPUTS: fd -- access to the RTC file
 *			 buf -- data buffer
 *			 nbytes -- the number of bytes of the
 *   OUTPUTS: none
 *   RETURN VALUE: nbytes, if data are successfully written
 * 				   -1, if the argument is invalid
 *   SIDE EFFECTS: none
 */
int32_t
rtc_write(int32_t fd, const void *buf, int32_t nbytes)
{
    if (nbytes != 4 || buf == NULL) // only accept 4 bytes input
        return -1;
    else
        rtc_set_freq(*((int32_t *)buf)); //take data stored in buf as input frequency

    return nbytes; //return the number of bytes written
}

/*
 * rtc_close
 *   DESCRIPTION: close a RTC file
 *   INPUTS: fd -- access to the RTC file
 *   OUTPUTS: none
 *   RETURN VALUE: always return 0
 *   SIDE EFFECTS: none
 */
int32_t
rtc_close(int32_t fd)
{
    //since we donot have related system calls, we do nothing.

    //rtc_set_freq(DEFAULT_FREQ); // restore the frequency to 2Hz by default

    return 0;
}

/*
 * rtc_interrupt_handler
 *   DESCRIPTION: handle RTC interrupt
 *   INPUTS: non
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_interrupt_handler(void)
{
    // printf("rtc_interrupt_handler\n");

    rtc_flag = CLEAR;
    outb(RTC_REGISTER_C, RTC_INDEX_PORT); // select register C
    inb(RTC_DATA_PORT);                   // just throw away contents
    rtc_flag = CLEAR;
}

/*
 * rtc_set_freq
 *   DESCRIPTION: set RTC interrupt frequency
 *   INPUTS: freq -- freqency
 *   OUTPUTS: none
 *   RETURN VALUE: 0, if the frequency is successfully set
 * 				   -1, if the argument is invalid
 *   SIDE EFFECTS: none
 */
int32_t
rtc_set_freq(int32_t freq)
{
    char rate;

    //rate is only allowed in the range of 3 to 15, and the system will restrict its interrupt rate up to 1024Hz
    if (freq < MIN_RTC_FREQ || freq > MAX_RTC_FREQ || !CHECK_POWER2(freq))
        return -1;
    else {
        //transform between frequency and rate: rate = log(frequency) + 1
        switch (freq) {
        case 2:
            rate = 0x0F;
            break;
        case 4:
            rate = 0x0E;
            break;
        case 8:
            rate = 0x0D;
            break;
        case 16:
            rate = 0x0C;
            break;
        case 32:
            rate = 0x0B;
            break;
        case 64:
            rate = 0x0A;
            break;
        case 128:
            rate = 0x09;
            break;
        case 256:
            rate = 0x08;
            break;
        case 512:
            rate = 0x07;
            break;
        case 1024:
            rate = 0x06;
            break;
        default:
            return -1;
        }

        //disable ints
        outb(RTC_DISABLE_NMI | RTC_REGISTER_A, RTC_INDEX_PORT); // set index to register A, disable NMI
        char prev = inb(RTC_DATA_PORT);                         // get initial value of register A
        outb(RTC_DISABLE_NMI | RTC_REGISTER_A, RTC_INDEX_PORT); // reset index to A
        outb((prev & 0xF0) | rate, RTC_DATA_PORT);              //write only our rate to A. Note, rate is the bottom 4 bits.
        //restore ints
        return 0;
    }
}

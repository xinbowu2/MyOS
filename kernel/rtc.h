/* rtc.h - the header file of rtc.c
*/

/* This defines port select and indexes of registers on RTC. 
*/

#include "types.h"

/*Maximum RTC Frequency */
#define MAX_RTC_FREQ 1024

/*Minimum RTC Frequency*/
#define MIN_RTC_FREQ 2

/*Default RTC Frequency*/
#define DEFAULT_FREQ 2

/*Initialize RTC*/
extern void init_rtc(void);

/*Open the RTC*/
extern int32_t rtc_open(const uint8_t *file_name);

/*Read the RTC*/
extern int32_t rtc_read(int32_t fd, void *buf, int32_t nbytes);

/*Write the RTC*/
extern int32_t rtc_write(int32_t fd, const void *buf, int32_t nbytes);

/*Close the RTC*/
extern int32_t rtc_close(int32_t fd);

/*Handle RTC interrupt*/
extern void rtc_interrupt_handler(void);

/*Set RTC Frequency*/
int32_t rtc_set_freq(int32_t freq);

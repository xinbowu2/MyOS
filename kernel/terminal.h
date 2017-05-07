#ifndef _TERMINAL
#define _TERMINAL

#include "types.h"

#define VIDEO 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25
#define ATTRIB 0x7
#define INDEX_REG 0x3D4
#define DATA_REG 0x3D5
#define LOW_PORT 0x0F
#define HIGH_PORT 0x0E
#define MAX_TERMINAL 3



/*output the buffer to the screen*/
int32_t terminal_write(const char *buf, int32_t nbytes);
/* internal helper that putc to a specific terminal */
void internel_terminal_putc(uint8_t c, int terminal);
/*write the Ascii value of the character to the video memory*/
void terminal_putc(uint8_t c);
/*Clear the screen and reset screen_x and screen_y*/
void terminal_clear();
/*Display a blue screen*/
void terminal_blue_screen();
/*Reset screen_x and screen_y to 0*/
void terminal_reset();
/*helper function for back space*/
int terminal_back_space();
/*Scroll the screen as well as new_line_checklist*/
void terminal_scroll();
/*Return -1*/
int32_t terminal_read();
/*Do nothing*/
int32_t terminal_open();
/*Do nothing*/
int32_t terminal_close();
/*Set the terminal cursor*/
void terminal_cursor(int row, int col);
// gets the cursor row by index
int get_cursor_row_index(int terminal);
// gets the cursor col by index
int get_cursor_col_index(int terminal);
/*Get the terminal cursor row position*/
int get_cursor_row();
/*Get the terminal cursor column position*/
int get_cursor_col();
/* fuzzes the screen */
void test_interrupts(void);
/*helper for back space. set the flag by index*/
void terminal_back_space_helper_internal(int index);
/*helper for back space. set the flag*/
void terminal_back_space_helper();
/* internal function for backspacing a specific terminal */
int terminal_back_space_internal(int terminal);
#endif

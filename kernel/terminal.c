#include "terminal.h"
#include "lib.h"
#include "schedule.h"


static char *video_mem = (char *)VIDEO;
static int screen_x[MAX_NUM_TERMINALS], screen_y[MAX_NUM_TERMINALS];
static int backspace_flag[MAX_NUM_TERMINALS];
static int newline_flag[MAX_NUM_TERMINALS];
int new_line_checklist[MAX_NUM_TERMINALS][NUM_ROWS];

void internel_terminal_putc(uint8_t c, int terminal) {
    int old_x = screen_x[terminal];
    /*if c is a newline, go to the next line*/
    if (c == '\n' || c == '\r') {
        screen_y[terminal]++;
        screen_x[terminal] = 0;
        /*if the next line goes out of the screen, scroll the screen*/
        if (screen_y[terminal] >= NUM_ROWS) {
            screen_y[terminal] = NUM_ROWS - 1;
            if (old_x == 0 && newline_flag[terminal] == 1) {
                newline_flag[terminal] = 0;
            } else {
                terminal_scroll();
            }
        }
        new_line_checklist[terminal][screen_y[terminal]] = 1;

    } else {
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y[terminal] + screen_x[terminal]) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y[terminal] + screen_x[terminal]) << 1) + 1) = ATTRIB;

        if (backspace_flag[terminal] == 1) {
            //if (c != 0)screen_x[terminal]++; //if the character is NUL, don't increment the screen_x[terminal]
            backspace_flag[terminal] = 0;
        } else {
            screen_x[terminal]++;
        }
        /*if position of the next character to print is outside of the screen
        scroll the screen*/
        if (screen_x[terminal] >= NUM_COLS) {
            //char_count = 1;
            screen_y[terminal]++;
            screen_x[terminal] = 0;
            /*if the next line goes out of the screen, scroll the screen*/
            if (screen_y[terminal] >= NUM_ROWS) {
                screen_y[terminal] = NUM_ROWS - 1;
                terminal_scroll();
                newline_flag[terminal] = 1;
            }
        }
    }

    if(visible_terminal == TERMINAL_INDEX) {
        terminal_cursor(screen_y[visible_terminal], screen_x[visible_terminal]);
    }
}
/*
terminal_putc
Description:
write the Ascii value of the character to the video memory
Input: character to be printed
*/
void terminal_putc(uint8_t c)
{
    internel_terminal_putc(c, TERMINAL_INDEX);
}

/*
terminal_write
Description: output the buffer to the screen; Call terminal_c
Input: The buffer to write, and number of bytes to be written
Return: the number of bytes written
*/
int32_t terminal_write(const char *buf, int32_t nbytes)
{
    int32_t i, bytes_count = 0;

    if (nbytes <= 0) {
        return -1;
    }

    for (i = 0; i < nbytes; i++) {
        terminal_putc(buf[i]);
        bytes_count++;
    }

    return bytes_count;
}

/*
terminal_clear
Description:
Clear the screen and reset screen_x[TERMINAL_INDEX] as well as screen_y[TERMINAL_INDEX]
*/
void terminal_clear()
{
    int32_t i;
    int loop_index;

    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
        //screen_x[TERMINAL_INDEX] = 0;
        //screen_y[TERMINAL_INDEX] = 0;
        screen_x[visible_terminal] = 0;
        screen_y[visible_terminal] = 0;
    }

    /*Clear the new_line_checklist[TERMINAL_INDEX]*/
    for (loop_index = 0; loop_index < NUM_ROWS; loop_index++) {
        //new_line_checklist[TERMINAL_INDEX][loop_index] = 0;
        new_line_checklist[visible_terminal][loop_index] = 0;

    }

    //new_line_checklist[TERMINAL_INDEX][0] = 1; //the first line after clear screen is a newline
    new_line_checklist[visible_terminal][0] = 1;
    terminal_cursor(0, 0);
}

/*
terminal_blue_screen
Description: Display a blue_screen
*/
void terminal_blue_screen()
{
    int32_t i;
    terminal_reset();
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        /*
	     *   7	       6   5   4	     3   2   1   0
	     * Blink   	Background color	Foreground color
	     */
        // F sets the text to white, 1 sets the screen to blue
        *(uint8_t *)(video_mem + (i << 1) + 1) = 0x1F;
    }
}

/*
terminal_reset
Description: reset screen_x[TERMINAL_INDEX] and screen_y[TERMINAL_INDEX] to 0
*/
void terminal_reset()
{
    screen_x[TERMINAL_INDEX] = 0;
    screen_y[TERMINAL_INDEX] = 0;
}

int terminal_back_space_internal(int terminal) {
    int flag = 0;
    /*move the screen_x[terminal] curser to the left by one char*/
    screen_x[terminal]--;

    /*if this is a "newline", you can not delete the whole row but only up to start of the line*/
    if (new_line_checklist[terminal][screen_y[terminal]]) {
        if (screen_x[terminal] < 0) {
            screen_x[terminal] = 0;
        } else {
            /*a char is deleted successfully*/
            //buf_in_ptr [Keyboard.buf_index] = 0;
            internel_terminal_putc(0, terminal);
            flag = 1;
        }
    }
    /*If this row is not a "newline" row, a whole row can be delted. And if this is not
    the first row in the screen, move the screen_y[TERMINAL_INDEX] up by one row */
    else {
        if (screen_x[terminal] < 0) {     /*if screen_x[TERMINAL_INDEX] is alreay 0 before moved to the left (which makes screen_x[TERMINAL_INDEX] negative now)*/
            if (screen_y[terminal] > 0) { /*if this is not the first row, move screen_y[TERMINAL_INDEX] upward by one row*/
                screen_y[terminal]--;
                screen_x[terminal] = NUM_COLS - 1; /*now screen_x[TERMINAL_INDEX] is at the right most of the screen*/
                internel_terminal_putc(0, terminal);
                flag = 1;
            } else {
                screen_x[terminal] = 0;
            }
        } else {
            /*a character is deleted successfully*/
            internel_terminal_putc(0, terminal);
            flag = 1;
        }
    }

    return flag;
}

/*
terminal_back_space
Description: helper function for back space
Return 1 if a character is succesfully deleted, 0 otherwise
*/
int terminal_back_space()
{
    return terminal_back_space_internal(TERMINAL_INDEX);
}

/*
terminal_scroll
Description: Scroll the screen as well as new_line_checklist[TERMINAL_INDEX]
*/
void terminal_scroll()
{
    int32_t i;
    /*Move everything on the screen one line above*/
    memcpy(video_mem, video_mem + NUM_COLS * 2, NUM_COLS * (NUM_ROWS - 1) * 2);

    /*Clear the bottom line of the screen*/
    for (i = (NUM_ROWS - 1) * NUM_COLS; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }

    /*Move the elements in new_line_checklist[TERMINAL_INDEX] one element above*/
    for (i = 1; i < NUM_ROWS; i++) {
        new_line_checklist[TERMINAL_INDEX][i - 1] = new_line_checklist[TERMINAL_INDEX][i];
    }

    /*Clear the last element */
    new_line_checklist[TERMINAL_INDEX][NUM_ROWS - 1] = 0;
}

/*
terminal_read
Description: return -1 since the terminal is write only
*/
int32_t terminal_read()
{
    return -1;
}

/*
terminal_open
Description: do nothing
*/
int32_t
terminal_open()
{
    return 0;
}

/*
terminal_close
Description: do nothing
*/
int32_t
terminal_close()
{
    return 0;
}

/*
terminal_cursor
Description: Set the terminal cursor at the position (row, col)
Input: the position of the cursor in row and column
*/
void terminal_cursor(int row, int col)
{
    unsigned short position = (row * NUM_COLS) + col;

    // cursor LOW port to vga INDEX register
    outb(LOW_PORT, INDEX_REG);
    outb((unsigned char)(position & 0xFF), DATA_REG); //0xFF is a mask

    // cursor HIGH port to vga INDEX register
    outb(HIGH_PORT, INDEX_REG);
    outb((unsigned char)((position >> 8) & 0xFF), DATA_REG); //0xFF is a mask
}

/*
* void test_interrupts(void)
*   Inputs: void
*   Return Value: void
*   Function: increments video memory. To be used to test rtc
*/
void test_interrupts(void)
{
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}
/*helper for back space. set the flag*/
void terminal_back_space_helper_internal(int index)
{
    backspace_flag[index] = 1;
}


/*helper for back space. set the flag*/
void terminal_back_space_helper()
{
    backspace_flag[TERMINAL_INDEX] = 1;
}

int get_cursor_row_index(int terminal) {
    return screen_y[terminal];
}

int get_cursor_col_index(int terminal){
    return screen_x[terminal];
}

/*Return the row index of the cursor*/
int get_cursor_row(){
    return screen_y[TERMINAL_INDEX];
}

/*Return the column index of the cursor*/
int get_cursor_col(){
    return screen_x[TERMINAL_INDEX];
}

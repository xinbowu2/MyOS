#include "keyboard.h"
#include "i8259.h"
#include "lib.h"
#include "terminal.h"
#include "schedule.h"

// static   //global keyboard buffer
// static  //global keyboard buffer
// static char *keyboards[visible_terminal].keyboard_buf_in = keyboard_buf_in;       //global keyboard buffer
// static char *keyboards[visible_terminal].keyboard_buf_out = keyboard_buf_out;     //global keyboard buffer

// one keyboard state per terminal
keyboard_t keyboards[MAX_NUM_TERMINALS];

/* This is a scancode table used to layout a standard US keyboard.*/
unsigned int uskm[USKM_SIZE] = {
        /*First Row*/
        0, 0 /*ESC*/, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0 /*'\b'*/,
        /*Second Row*/
        /*'\t'*/ 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        /*third row*/
        0, /* 29   - Control */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        /*Forth row*/
        0,                                                         /* Left shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, /* Right shift */

        '*',
        0,   /* Right Alt */
        ' ', /* Space bar */
        0,   /* Caps lock */
        0,   /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0,
        0,                                    /* F11 Key */
        0,                                    /* F12 Key */
        0, /* All other keys are undefined */ /*89*/

        //shift + table

        /*first row*/
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
        /*Second Row*/
        0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,
        /*third row*/
        0, /* 29   - Control */
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
        /*Forth row*/
        0,                                                        /* Left shift */
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, /* Right shift */
};

char* get_keyboard_in_buf(){
    return keyboards[visible_terminal].keyboard_buf_in;
}


/*
flush
Description: flush the keyboard buffers according to the selection
Input: An integer No to select which buffer (of both) to flush:
If No == 0, flush buf_in; if No == 1, flush buf_out; if No == 2, sflush both keyboard buffers
*/
void flush(int No)
{
    if ((No == 0) | (No == 2)) {
        memset(keyboards[visible_terminal].keyboard_buf_in, 0, keyboard_buf_size);
    }

    if ((No == 1) | (No == 2)) {
        memset(keyboards[visible_terminal].keyboard_buf_out, 0, keyboard_buf_size);
    }
    keyboards[visible_terminal].buf_index = 0;
}

/*
 * keyboard_open
 *   DESCRIPTION: flush the buffers when the keyboard is open
 */
int32_t
keyboard_open()
{
    /*Initialize the keyboard buffer to be an array of 'NUL'*/
    flush(2);

    return 0;
}

/*
 * keyboard_close
 *   DESCRIPTION: flush the keyboard buffers when the keyboard is closed
 */
int32_t
keyboard_close()
{
    /*set the keyboard buffer to be an array of 'NUL'*/
    flush(2);

    return 0;
}

/*
 * keyboard_read
 *   DESCRIPTION:  read should return data from one line that has been terminated by pressing Enter,
 *   or as much as fits in the buffer from one such line.
 *   INPUT: the buffer to read from the keyboard buffers and the number of bytes to be read
 */
int32_t keyboard_read(char *buf, int32_t nbytes)
{
    int32_t i = 0;
    //printf("Hi I just entered\n");

    if (nbytes <= 0) {
        return -1;
    }

    while (!keyboards[visible_terminal].keyboard_buff_to_go || (TERMINAL_INDEX != visible_terminal)) {
        /*wait for enter to be pressed*/
    };
    //printf("I just left the loop\n");

    /* Copy over everything until the newline character*/
    for (i = 0; i < nbytes; i++) {
        if (i == keyboard_buf_size) {
            buf[i] = '\n';
            i++;
            break;
        }

        // buf[i] = *(keyboards[visible_terminal].keyboard_buf_out+i);
        buf[i] = keyboards[visible_terminal].keyboard_buf_out[i];

        // printf("hi%x ",buf[i]);
        if (buf[i] == '\n') {
            i++;
            break;
        }
    }

    //clear keyboard_buf_out
    flush(1);

    keyboards[visible_terminal].keyboard_buff_to_go = 0;
    return i;
}

/*
 * keyboard_write
 *   DESCRIPTION: return -1 since the keyboard is read only
 */
int32_t
keyboard_write(int32_t fd, const void *buf, int32_t nbytes)
{
    return -1;
}

/*
keyboard_init()
Description: Initializes globals for keyboard. Primarly initializes stuff in keyboards[visible_terminal].
 */
void keyboard_init()
{
    /*Init the keyboard*/
    // keyboards[visible_terminal].shift = 0;
    // keyboards[visible_terminal].caps_lock = 0;
    // keyboards[visible_terminal].control = 0;
    // keyboards[visible_terminal].alt = 0;
    // keyboards[visible_terminal].keyboard_buff_to_go = 0;
    // keyboards[visible_terminal].buf_index = 0;
    // /*Clear the keyboard buffers*/
    // flush(2);
    /*Clear the screen and reset screen_x and screen_y*/
    clear();
    /*Enable keyboard interrupts*/
    enable_irq(1);
}

// echos the character to the terminal
void echo(uint8_t c) {
    unsigned long flags;
    cli_and_save(flags);
    // switching to kernel pd so we can write directory to video memory
    SET_CR3(&pd_kernel[0]);
    internel_terminal_putc(c, visible_terminal);
    terminal_cursor(get_cursor_row_index(visible_terminal), get_cursor_col_index(visible_terminal));
    // switching back to the scheduled process
    SET_CR3(&pds[curr_pid]);
    restore_flags(flags);
}

/*
 * back_space_helper
 *   DESCRIPTION: Handle the back pace, i.e. the delte key on Mac
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: delete the previous character.
 */
void back_space_helper()
{
    // Make sure the user can't backspace if nothing is in the buffer
    if (keyboards[visible_terminal].buf_index == 0) {
        return;
    }

    terminal_back_space_helper_internal(visible_terminal);
    /*Decrement the keyboard buffer index if a character is successfully deleted*/
    if (terminal_back_space_internal(visible_terminal)) {
        keyboards[visible_terminal].buf_index--;
    }
    terminal_cursor(get_cursor_row_index(visible_terminal), get_cursor_col_index(visible_terminal));
}

/*
 * other_key_helper
 *   DESCRIPTION: Handle keys that are not function keys (ctr, shft, or caps lock)
 *   INPUTS: scan code of the key
 *   OUTPUTS:
 *   RETURN VALUE: none
 *   SIDE EFFECTS: according to the combination of this key and the previously received key,
 *   either echo the character or perfom some certain function. For example, if it is CTR+L,
 *   clear the screen. If either caps lock or shft is on (but not both of them on), echo the capital
 *   letter. If the received key is not a letter, Only echo the "upper case" of the key when shft is on
 */
void other_keys_helper(unsigned int scan)
{
    char temp[keyboard_buf_size];
    //echo mode:
    if (!keyboards[visible_terminal].control && !keyboards[visible_terminal].alt) {
        /*Preprocess the scancode*/
        /*if it is a back space*/
        if (scan == BACK_SPACE) {
            back_space_helper();
            return;
        }
        /*if it is a letter*/
        else if ((scan >= LETTERS_SECOND_START && scan <= LETTERS_SECOND_END) ||
                 (scan >= LETTERS_THIRD_START && scan <= LETTERS_THIRD_END) ||
                 (scan >= LETTERS_FOURTH_START && scan <= LETTERS_FOURTH_END)) {
            /*if cpas_lock and shift are not both on, print the upper case letter*/
            if (keyboards[visible_terminal].caps_lock != keyboards[visible_terminal].shift) {
                scan += ECHO_OFFSET;
            }
        }
        /*For non-letter character, only echo the "upper case" if shift is on*/
        else if (keyboards[visible_terminal].shift) {
            scan += ECHO_OFFSET;
        }
        if (scan >= USKM_SIZE || scan == 1 || uskm[scan] == 0) {
            //echo(0);
            return;
        }

        /*Echo and write the char to the buffer*/
        /*if the buffer is not full*/
        if (keyboards[visible_terminal].buf_index < keyboard_buf_size) {
            /*echo to the screen if it is not '\n'*/
            if (uskm[scan] != '\n') {
                echo(uskm[scan]);
            }
            //printf("%c", uskm[scan]);

            /*write to the kyeboard buffer
                ('\n' is written to the keyboard buffer too)*/
            keyboards[visible_terminal].keyboard_buf_in[keyboards[visible_terminal].buf_index] = uskm[scan];

            keyboards[visible_terminal].buf_index++;
        } else {
            // printf("\nbuffer is full. press enter to flush it\n");
        }

        /*if newline is entered*/
        if (uskm[scan] == '\n') {
            echo('\n');

            keyboards[visible_terminal].buf_index = 0;

            /*switch the buffer pointer
                (if the user spams enter, it might lose packets)*/
            memcpy(temp, keyboards[visible_terminal].keyboard_buf_in, keyboard_buf_size);
            memcpy(keyboards[visible_terminal].keyboard_buf_in, keyboards[visible_terminal].keyboard_buf_out, keyboard_buf_size);
            memcpy(keyboards[visible_terminal].keyboard_buf_out, temp, keyboard_buf_size);
            keyboards[visible_terminal].keyboard_buff_to_go = 1;
        }

    }
    /*function mode*/
    else {
        if (keyboards[visible_terminal].control) {
            scan += CONTROL_OFFSET;
        }
        if (keyboards[visible_terminal].shift) {
            scan += SHIFT_OFFSET;
        }
        if (keyboards[visible_terminal].alt) {
            scan += ALT_OFFSET;
        }
        /*Perform the corresponding function*/
        switch (scan) {
        case CTR_L_MAGIC:
            //  char_count = 0;
            /*Clear the video memory as well as reset screen_x and screen_y*/
            clear();
            /*clear the keyboard buffers*/
            flush(2);
            keyboards[visible_terminal].keyboard_buff_to_go = 1;
            break;
        case ALT_F1_MAGIC:
            switch_visible_terminal(0);
            break;
        case ALT_F2_MAGIC:
            switch_visible_terminal(1);
            break;
        case ALT_F3_MAGIC:
            switch_visible_terminal(2);
            break;
        default:
            break;
        }
    }
}

/*
keyboard_driver
Description: handle the keyboard interrupt
*/
void keyboard_driver()
{
    unsigned long flags;
    cli_and_save(flags);
    // switching to kernel pd so we can write directory to video memory
    SET_CR3(&pd_kernel[0]);

    unsigned int scan;
    unsigned int i;
    scan = inb(KEYBOARD_IN); //at this moment the buffer of the keyboard is cleared by the
                             //keyboard controller in the motherboard

    i = inb(OLD_PORT);
    outb(i | SCANCODE_MASK, OLD_PORT); //use 0x80 to disable the keyboard
    outb(i, OLD_PORT);                 //enable the keyboard

    /*if a key is pressed*/
    if (!(scan & SCANCODE_MASK)) { //scan & 0x80 is to mask out the scan code, so the released scan code
                                   //will be 0, pressed scan code is not 0
        /*if PRE_KEY is received, do nothing*/
        if (scan != PRE_KEY) {
            /*shift key is pressed*/
            switch (scan) {
            case L_SHIFT_KEY:
                /*set the shift flag if it is not already set*/
                if (!keyboards[visible_terminal].shift) keyboards[visible_terminal].shift = 1;
                break;
            case R_SHIFT_KEY:
                /*set the shift flag if it is not already set*/
                if (!keyboards[visible_terminal].shift) keyboards[visible_terminal].shift = 1;
                break;
            /*caps lock is pressed*/
            case CAPSLOCK_KEY:
                /*set the caps lock to the opposite value*/
                if (keyboards[visible_terminal].caps_lock == 1) {
                    keyboards[visible_terminal].caps_lock = 0;
                } else {
                    keyboards[visible_terminal].caps_lock = 1;
                }
                break;
            /*ctr key is pressed*/
            case CONTROL_KEY:
                /*set the control flag if it is not already set*/
                if (!keyboards[visible_terminal].control) keyboards[visible_terminal].control = 1;
                break;
            /*ALT key is pressed*/
            case ALT_KEY:
                /*set the alt flag if it is not already set*/
                if (!keyboards[visible_terminal].alt) keyboards[visible_terminal].alt = 1;
                break;
            default: /*other keys are pressed*/
                other_keys_helper(scan);
                break;
            }
        }
    }

    /*if the key is released*/
    else {
        switch (scan) {
        /*if shift key is released*/
        case L_SHIFT_BR_KEY:
            /*clear the shift flag if it is not already cleared*/
            if (keyboards[visible_terminal].shift) keyboards[visible_terminal].shift = 0;
            break;
        /*if shift key is released*/
        case R_SHIFT_BR_KEY:
            /*clear the shift flag if it is not already cleared*/
            if (keyboards[visible_terminal].shift) keyboards[visible_terminal].shift = 0;
            break;
        case CONTROL_BR_KEY: /*if control key is released*/
                             /* clear the control flag if it is not already cleared*/
            if (keyboards[visible_terminal].control) keyboards[visible_terminal].control = 0;
            break;
        case ALT_BR_KEY: /*if ALT key is released*/
                             /* clear the alt flag if it is not already cleared*/
            if (keyboards[visible_terminal].alt) keyboards[visible_terminal].alt = 0;
            break;
        default:
            break;
        }
    }

    // switching back to the scheduled process
    SET_CR3(&pds[curr_pid]);
    restore_flags(flags);
}

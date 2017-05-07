/*Reference: http://www.osdever.net/bkerndev/Docs/keyboard.htm*/

/*Magic number used to determine if the system needs to perform some special
functionality if some keys (or combination of keys) are pressed. The magic
number need to be bigger than the size of the keymap, so nothing will be
printed out (if printing is not part of the desired functionality)
*/

#ifndef _KEYBOARD
#define _KEYBOARD

#include "types.h"

#define KEYBOARD_IN 0x60
#define OLD_PORT 0x61
#define SCANCODE_MASK 0x80
#define keyboard_buf_size 128
#define USKM_SIZE 145
#define TAB_CODE 15

/*Some big number I made up, inorder to tell apart different combination of
function keys along with value keys*/
#define CONTROL_OFFSET 0x7F00
#define SHIFT_OFFSET 0x7E00
#define ALT_OFFSET 0x7D00
#define ECHO_OFFSET 0x5A


#define LETTERS_SECOND_START 0x10 //offset from 'NUL' to the first letter (i.e. 'q') in the second row
#define LETTERS_SECOND_END 0x19   //offset from 'NUL' to the last letter (i.e. 'p') in the second row
#define LETTERS_THIRD_START 0x1E  //offset from 'NUL' to the first letter (i.e. 'q') in the third row
#define LETTERS_THIRD_END 0x26    //offset from 'NUL' to the last letter (i.e. 'p') in the third row
#define LETTERS_FOURTH_START 0x2C //offset from 'NUL' to the first letter (i.e. 'q') in fourth first row
#define LETTERS_FOURTH_END 0x32   //offset from 'NUL' to the last letter (i.e. 'p') in the fourth row

#define BACK_SPACE 0x0E
#define L_KEY 0x26
#define F1_KEY 0x3B
#define F2_KEY 0x3C
#define F3_KEY 0x3D


#define CTR_L_MAGIC 0x7F26 //CONTROL_OFFSET+'L'
#define ALT_F1_MAGIC 0x7D3B //ALT_OFFSET+'F1'
#define ALT_F2_MAGIC 0x7D3C //ALT_OFFSET+'F2'
#define ALT_F3_MAGIC 0x7D3D //ALT_OFFSET+'F3'

/*scancode for function keys*/
#define PRE_KEY 0xE0
#define CAPSLOCK_KEY 0x3A
#define L_SHIFT_KEY 0x2A
#define R_SHIFT_KEY 0x36
#define CONTROL_KEY 0x1D
#define L_SHIFT_BR_KEY 0xAA
#define R_SHIFT_BR_KEY 0xB6
#define CONTROL_BR_KEY 0x9D
#define ALT_KEY 0x38
#define ALT_BR_KEY 0xB8

// Struct to maintain the state of a keyboard
typedef struct keyboard_struct {
    /*Flags for special keys*/
    int caps_lock;
    int shift;
    int alt;
    int control;
    int buf_index;
    int keyboard_buff_to_go;
    char keyboard_buf_in[keyboard_buf_size];
    char keyboard_buf_out[keyboard_buf_size];
} keyboard_t;

extern unsigned int uskm[145]; //an array of ascii coressdponding to the scan code

/* Initializes globals for keyboard. Primarly initializes stuff in Keyboard.*/
void keyboard_init();
/*Keyboard driver function: used to handle the keyboard interrupt*/
void keyboard_driver();

/*helper functions:*/
/*back_space_helper handles the delete keys*/
void back_space_helper();
/*other_key_helper handles keys that are not function keys nor the delete key*/
void other_key_helper(unsigned int scan);
/*flush the keyboard buffers according to the selection*/
void flush(int No);
/*flush the keyboard when keyboard is open*/
int32_t keyboard_open();
/*flush the keyboard when keyboard is close*/
int32_t keyboard_close();
/*Read from the keyboard buffer to the buffer*/
int32_t keyboard_read(char *buf, int32_t nbytes);
/*return -1*/
int32_t keyboard_write(int32_t fd, const void *buf, int32_t nbytes);
/*Return the address of the keyboard buffer*/
char* get_keyboard_in_buf();
#endif

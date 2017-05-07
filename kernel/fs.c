#include "fs.h"
#include "keyboard.h"
#include "pcb.h"
#include "rtc.h"
#include "terminal.h"
#include "pt.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* opens a file successfully */
int32_t open_success(const uint8_t *filename)
{
    return 0;
}

/* fails to read a file successfully */
int32_t read_fail(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    return -1;
}

/* fails to write to a file successfully */
int32_t write_fail(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    return -1;
}

/* closes a file successfully */
int32_t close_success(int32_t fd)
{
    return 0;
}

/* wrappers for rtc file operations */
int32_t rtc_read_wrapper(uint32_t id, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    // throwing away offset, since rtc is not a seekable file
    (void)offset; 
    return rtc_read(id, (void *)buf, nbytes);
}

int32_t rtc_write_wrapper(uint32_t id, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    // throwing away offset, since rtc is not a seekable file
    (void)offset;
    return rtc_write(id, (void *)buf, nbytes);
}

/*
 * The file operator table for every type of file in the read only file system
 */
file_operator_table_t file_operator_tables[NUM_FILE_TYPES] = {
        // 0 = RTC operators
        {
                rtc_open,
                rtc_read_wrapper,
                rtc_write_wrapper,
                rtc_close},
        // 1 = Directory File Operators
        {
                open_success,
                read_directory,
                write_fail,
                close_success},
        // 2 = Regular File Operators
        {
                open_success,
                read_data,
                write_fail,
                close_success}};

/* wrappers for keyboard operations */
int32_t keyboard_open_wrapper(const uint8_t *filename)
{
    (void)filename; // throwing this away since the keyboard does not have a file name.
    return keyboard_open();
}

int32_t keyboard_read_wrapper(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    (void)offset; // keyboard is not a seekable file
    (void)inode;  // keyboard does not have an inode;
    return keyboard_read((char *)buf, nbytes);
}

int32_t keyboard_write_wrapper(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    (void)offset; // keyboard is not a seekable file
    // (void) inode; // keyboard does not have an inode;
    return keyboard_write(inode, (char *)buf, nbytes);
}

int32_t keyboard_close_wrapper(int32_t fd)
{
    (void)fd;
    return keyboard_close();
}

/*
 * 'stdin' is a read-only file which corresponds to keyboard input.
 */
file_operator_table_t stdin_file_operator_table = {
        keyboard_open_wrapper,
        keyboard_read_wrapper,
        keyboard_write_wrapper,
        keyboard_close_wrapper};

/*wrappers for terminal operations*/
int32_t terminal_open_wrapper(const uint8_t *filename)
{
    (void)filename; // throwing this away since the terminal does not have a file name.
    return terminal_open();
}

int32_t terminal_read_wrapper(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    (void)offset; // terminal is not a seekable file
    (void)inode;  // terminal does not have an inode
    (void)buf;    // terminal is not readable
    (void)nbytes; // terminal is not readable
    return terminal_read();
}

int32_t terminal_write_wrapper(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t nbytes)
{
    (void)offset; // terminal is not a seekable file
    // (void) inode; // terminal does not have an inode;
    return terminal_write((char *)buf, nbytes);
}

int32_t terminal_close_wrapper(int32_t fd)
{
    (void)fd; // terminal does not have a fd
    return terminal_close();
}

/*
 * 'stdout' is a write-only file corresponding to terminal output.
 */
file_operator_table_t stdout_file_operator_table = {
        terminal_open_wrapper,
        terminal_read_wrapper,
        terminal_write_wrapper,
        terminal_close_wrapper};

/*
 * When a process is started the kernel should automatically open 'stdin' and 'stdout',
 * which correspond to file descriptors 0 and 1, respectively.
 */

/*
 * Opens stdin at STDIN_FILENO in current process (as determined by curr_pid).
 */
void open_stdin()
{
    pcb_entry_t *pcb_entry = GET_PCB_ENTRY(curr_pid);
    file_t *files = pcb_entry->files;

    files[STDIN_FILENO] = (file_t){
            &stdin_file_operator_table,
            NULL,
            0,
            IN_USE};

    stdin_file_operator_table.open(NULL);
}

/*
 * Opens stdout at STDOUT_FILENO in current process (as determined by curr_pid).
 */
void open_stdout()
{
    pcb_entry_t *pcb_entry = GET_PCB_ENTRY(curr_pid);
    file_t *files = pcb_entry->files;

    files[STDOUT_FILENO] = (file_t){
            &stdout_file_operator_table,
            NULL,
            0,
            IN_USE};

    stdout_file_operator_table.open(NULL);
}

void init_fs(char *addr)
{
    fs = (block_t *)addr;
    boot_block = (boot_block_t *)fs;
    inodes = (inode_t *)((boot_block_t *)fs + 1);
    data_blocks = (block_t *)(inodes + boot_block->num_inodes);
}

int32_t read_dentry_by_name(const uint8_t *file_name, dentry_t *dentry)
{
    dentry_t *entries = boot_block->dir_entries;
    int i;

    for (i = 0; i < boot_block->num_directory_entries; i++) {
        if (strncmp((const int8_t *)(&(entries[i]))->file_name, (const int8_t *)file_name, FILE_NAME_LENGTH) == 0) {
            memcpy(dentry, &entries[i], sizeof(dentry_t));
            return 0;
        }
    }

    return -1;
}

int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry)
{
    dentry_t *entries = boot_block->dir_entries;
    if (index >= boot_block->num_directory_entries) {
        return -1;
    }

    memcpy(dentry, &entries[index], sizeof(dentry_t));
    return 0;
}

int32_t read_data(uint32_t inode_number, uint32_t offset, uint8_t *buf, uint32_t length)
{
    inode_t *inode = inodes + inode_number;
    uint32_t bytes_remaining = MIN(length, inode->length);
    uint32_t *data_blocks_numbers = inode->data_blocks;
    int i;
    int start_block_index;
    int end_block_index;

    if (inode_number >= boot_block->num_inodes) {
        return -1;
    }

    if (offset >= inode->length) {
        return 0;
    }

    start_block_index = offset / BLOCK_SIZE;
    end_block_index = MIN(offset + length, inode->length) / sizeof(block_t);
    offset -= start_block_index * BLOCK_SIZE;

    // Copy over what is remaining of the start block
    memcpy(buf, (char *)(data_blocks + data_blocks_numbers[start_block_index]) + offset, MIN(bytes_remaining, sizeof(block_t) - offset));
    buf += MIN(bytes_remaining, sizeof(block_t) - offset);
    bytes_remaining -= MIN(bytes_remaining, sizeof(block_t) - offset);

    // Copy over the middle blocks
    for (i = start_block_index + 1; i < end_block_index; i++) {
        memcpy(buf, data_blocks + data_blocks_numbers[i], sizeof(block_t));
        buf += sizeof(block_t);
        bytes_remaining -= sizeof(block_t);
    }

    // Copy over the end block
    memcpy(buf, (char *)(data_blocks + data_blocks_numbers[end_block_index]), bytes_remaining);

    return MIN(length, inode->length);
}

int32_t read_directory(uint32_t inode_number, uint32_t offset, uint8_t *buf, uint32_t length)
{
    uint32_t bytes_remaining = length;
    dentry_t *dir_entries = boot_block->dir_entries;
    int start_index = offset / FILE_NAME_LENGTH;
    int end_index = MIN(offset + length, boot_block->num_directory_entries * FILE_NAME_LENGTH) / FILE_NAME_LENGTH;
    int i;
    (void)inode_number;

    if (offset >= boot_block->num_directory_entries * FILE_NAME_LENGTH) {
        return 0;
    }

    offset -= start_index * FILE_NAME_LENGTH;

    // Copy over what is remaining of the start filename
    memcpy(buf, (char *)((dir_entries + start_index)->file_name + offset), MIN(bytes_remaining, FILE_NAME_LENGTH - offset));
    buf += MIN(bytes_remaining, FILE_NAME_LENGTH - offset);
    bytes_remaining -= MIN(bytes_remaining, FILE_NAME_LENGTH - offset);

    // Copy over the middle filenames
    for (i = start_index + 1; i < end_index; i++) {
        memcpy(buf, (dir_entries + i)->file_name, FILE_NAME_LENGTH);
        buf += FILE_NAME_LENGTH;
        bytes_remaining -= FILE_NAME_LENGTH;
    }

    // Copy over the end block
    memcpy(buf, (char *)(dir_entries + end_index), bytes_remaining);

    return MIN(length, boot_block->num_directory_entries * FILE_NAME_LENGTH);
}

int32_t sys_open(const uint8_t *filename)
{
    int32_t fd = -1;
    int32_t i;
    pcb_entry_t *curr_pcb_entry = GET_PCB_ENTRY(curr_pid);
    file_t *files = curr_pcb_entry->files;
    dentry_t dir_entry;
    int32_t retval;

    for (i = 0; i < MAX_FILES_PER_PROCESS; i++) {
        if ((&files[i])->flags == AVAILABLE) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        return -1;
    }

    if (read_dentry_by_name(filename, &dir_entry) == -1) {
        return -1;
    }

    files[fd] = (file_t){
            file_operator_tables + dir_entry.file_type,
            inodes + dir_entry.inode_number,
            0,
            IN_USE};

    if ((retval = (&files[fd])->file_operations_table_pointer->open(filename))) {
        return retval;
    }

    return fd;
}

int32_t sys_read_write_helper(int32_t fd, const void *buf, int32_t nbytes, int file_operator)
{
    if (fd < 0 || fd > MAX_FILES_PER_PROCESS) {
        return -1;
    }

    if (buf == NULL || !is_user((uint32_t)buf)){
        return -1;
    }

    pcb_entry_t *curr_pcb_entry = GET_PCB_ENTRY(curr_pid);
    file_t *file = curr_pcb_entry->files + fd;
    int32_t retval;
    read_write_callback callback;

    if (file->flags != IN_USE) {
        return -1;
    }

    callback = file_operator == READ ? file->file_operations_table_pointer->read : file->file_operations_table_pointer->write;

    retval = callback(
            /*
         * When two pointers are subtracted, both shall point to elements of the same array object,
         * or one past the last element of the array object;
         * the result is the difference of the subscripts of the two array elements.
         */
            file->inode_pointer - inodes,
            file->file_position,
            (void *)buf,
            nbytes);

    if (retval == -1) {
        return -1;
    }

    file->file_position += retval;
    return retval;
}

int32_t sys_read(int32_t fd, void *buf, int32_t nbytes)
{
    return sys_read_write_helper(fd, buf, nbytes, READ);
}

int32_t sys_write(int32_t fd, const void *buf, int32_t nbytes)
{
    return sys_read_write_helper(fd, buf, nbytes, WRITE);
}

int32_t sys_close(int32_t fd)
{
    pcb_entry_t *curr_pcb_entry = GET_PCB_ENTRY(curr_pid);
    file_t *file = curr_pcb_entry->files + fd;
    int retval;

    if(file->flags != IN_USE) {
        return -1;
    }

    // STDIN and STDOUT are 0 and 1 so skipping over
    if (fd < STDOUT_FILENO + 1 || fd > MAX_FILES_PER_PROCESS) {
        return -1;
    }

    retval = file->file_operations_table_pointer->close(fd);

    memset(file, 0x00, sizeof(file));
    file->flags = AVAILABLE;

    return retval;
}

int32_t lseek(int32_t fd, int32_t offset, int32_t whence)
{
    pcb_entry_t *curr_pcb_entry = GET_PCB_ENTRY(curr_pid);
    file_t *file = curr_pcb_entry->files + fd;

    switch (whence) {
    case SEEK_SET:
        file->file_position = offset;
        break;
    case SEEK_CUR:
        file->file_position += offset;
    default:
        return -1;
    }

    return file->file_position;
}

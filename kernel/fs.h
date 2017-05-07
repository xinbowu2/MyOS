#ifndef _FS_H
#define _FS_H

#include "lib.h"
#include "types.h"

// The file system memory is divided into 4 kB blocks.
#define BLOCK_SIZE 4096
#define FILE_NAME_LENGTH 32

// Forward Declaring Structs
typedef struct block block_t;
typedef struct dir_entry dentry_t;
typedef struct index_node inode_t;
typedef struct boot_block boot_block_t;

/* Useful Globals */
// Pointer to the begining of the file system
block_t *fs;
// Pointer to the boot_block
boot_block_t *boot_block;
// Pointer to the first inode
inode_t *inodes;
// Pointer to the first data block
block_t *data_blocks;

// struct the same size as a block on disk for pointer arthimetic
struct block {
    char bytes[BLOCK_SIZE];
};

typedef enum file_type {
    // File types are 0 for a file giving user-level access to the real-time clock (RTC).
    ULA_RTC,
    // 1 for the directory
    DIRECTORY,
    // 2 for a regular file
    REGULAR,
    // enum trick to get the number of elements
    NUM_FILE_TYPES
} file_type_enum;

// Each directory entry gives:
struct dir_entry {
    // a name (up to 32 characters, zero-padded, but not neccessarily including a terminal EOS or 0-byte)
    uint8_t file_name[FILE_NAME_LENGTH];
    // a file type
    file_type_enum file_type;
    // an index node number for the file
    uint32_t inode_number;
    uint8_t reserved[24];
};

// Each regular file is described by an index node that specifies:
struct index_node {
    // The file's size in bytes
    uint32_t length;
    // and the data blocks that makeup the file
    uint32_t data_blocks[(BLOCK_SIZE - sizeof(uint32_t)) / sizeof(uint32_t)];
};

/*
 * The first block is called the boot block, and holds both file system statistis and the diretory entries.
 * Both the statistis and each diretory entry occupy 64B, so the file system can hold up to 63 files.
 * The first directory entry always refers to the directory itself, and is named ".", so it can really hold only 62 files.
 */
struct boot_block {
    uint32_t num_directory_entries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t reserved[52];
    dentry_t dir_entries[63];
};

// File descriptor macros
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

// Typedef'ing file function callbacks
typedef int32_t (*open_callback)(const uint8_t *filename);
typedef int32_t (*read_write_callback)(uint32_t id, uint32_t offset, uint8_t *buf, uint32_t nbytes);
typedef int32_t (*close_callback)(int32_t fd);

typedef struct file_operator_table {
    open_callback open;
    read_write_callback read;
    read_write_callback write;
    close_callback close;
} file_operator_table_t;

// File Operator Enum
typedef enum file_operator {
    OPEN,
    READ,
    WRITE,
    CLOSE,
    NUM_FILE_OPERATIONS
} file_operator_enum;

// Stores the information needed to read from a file
typedef struct file {
    /*
     * The file operations jump table associated with the correct file type.
     * The jump table should contain entries for 'open', 'read', 'write', 'close' to perform type-specific actions for each operation.
     * 'open' is used for performing type-specific initialization.
     * For example, if we just 'open'd the RTC, the jump table pointer in this structure should store the RTC's file operations table.
     */
    file_operator_table_t *file_operations_table_pointer;
    /*
     * A pointer to the inode for this file.
     * This is only valid for data files, and should be NULL for directories and the RTC device file.
     */
    inode_t *inode_pointer;
    /*
     * A "file position" member that keeps track of where the user is currently reading from the file.
     * Every 'read' system call should update this member.
     */
    uint32_t file_position;
    /*
     * A "flags" member for, among other things, marking the file descriptor as "in-use"
     */
    uint32_t flags;
} file_t;

// File Flag Enum
typedef enum file_flag {
    AVAILABLE,
    IN_USE,
    NUM_FILE_FLAGS
} file_flag_enum;

// Whence Enum for lseek()
typedef enum whence {
    SEEK_SET,
    SEEK_CUR,
    NUM_WHENCE_TYPES
} whence_enum;

/* Opens stdin for the current process*/
void open_stdin();

/* Opens stdout for the current process*/
void open_stdout();

/* initializes the filesystem with the start address in memory */
void init_fs(char *fs_addr);

/*
 * On Success: Fills in the dentry_t block passed as 'dentry' with the
 *      file name,
 *      file type,
 *      and inode number for the file,
 * matching 'file_name', then returns 0.
 * On Failure: returns -1 indicating a non-existent file.
 */
int32_t read_dentry_by_name(const uint8_t *file_name, dentry_t *dentry);

/*
 * On Success: Fills in the dentry_t block passed as 'dentry' with the
 *      file name,
 *      file type,
 *      and inode number for the file,
 * matching the 'index', then returns 0.
 * On Failure: returns -1 indicating an invalid index.
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry);

/*
 * Reads up to 'length' bytes starting from position 'offset' in the file with inode number 'inode'
 * and returns the number of bytes read and placed into the buffer.
 * A return value of 0 thus indicates that the end of file has been reached.
 * On failure -1 is returned meaning an invalid inode number was given.
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);

/*
 * Reads a directory as if all the filenames in the directory (including ".") were concatenated into one big file
 *
 * Note: inode_number is ignored and is only there to match the function header of 'read_data'.
 */
int32_t read_directory(uint32_t inode_number, uint32_t offset, uint8_t *buf, uint32_t length);

/*
 * Kernel entry point for opening a file:
 *
 * Given a 'filename' for a file, sys_open() returns a file descriptor,
 * a small, nonnegative integer for use in subsequent system calls (sys_read, sys_write, sys_close).
 * The file descriptor returned by a successful call will be the lowest-numbered file descriptor not currently open for the process.
 * On error this function returns -1.
 */
int32_t sys_open(const uint8_t *filename);

/*
 * Kernel entry point for reading a file:
 *
 * sys_read() attempts to read up to 'nbytes' bytes from file descriptor 'fd' into the buffer starting at 'buf.
 * On files that support seeking, the read operation commences at the current file offset,
 * and the file offset is incremented by the number of bytes read.
 * If the current file offset is at or past the end of file, no bytes are read, and sys_read() returns zero.
 *
 * On success, the number of bytes read is returned (zero indicates end of file),
 * and the file position is advanced by this number.
 * It is not an error if this number is smaller than the number of bytes requested;
 * this may happen for example because fewer bytes are actually available right now
 * (maybe because we were close to end-of-file, or because we are reading from a pipe, or from a terminal),
 * or because sys_read() was interrupted by a signal.
 * On error, -1 is returned.
 * In this case it is left unspecified whether the file position (if any) changes.
 */
int32_t sys_read(int32_t fd, void *buf, int32_t nbytes);

/*
 * Kernel entry point for writing to a file:
 *
 * sys_write() writes up to 'nbytes' bytes from the buffer pointed by 'buf' to the file referred to by the file descriptor 'fd'.
 * The number of bytes written may be less than count if, for example,
 * there is insufficient space on the underlying physical medium,
 * or the call was interrupted by a signal handler after having written less than count bytes.
 *
 * For a seekable file writing takes place at the current file offset,
 * and the file offset is incremented by the number of bytes actually written.
 * The file offset is first set to the end of the file before writing.
 * The adjustment of the file offset and the write operation are performed as an atomic step.
 */
int32_t sys_write(int32_t fd, const void *buf, int32_t nbytes);

/*
 * Kernel entry point for writing to a file:
 *
 * sys_close() closes a file descriptor, so that it no longer refers to any file and may be reused.
 * returns zero on success. On error, -1 is returned.
 */
int32_t sys_close(int32_t fd);

/*
 * The lseek() function repositions the offset of the open
 * file associated with the file descriptor fd to the argument
 * offset according to the directive whence as follows:
 *
 *      SEEK_SET - The offset is set to offset bytes.
 *      SEEK_CUR - The  offset is set to its current location plus off‐set bytes.
 *
 * Upon successful completion,
 * lseek() returns the resulting offset location as measured in bytes from the beginning of the file.
 * On error, the value (off_t) -1 is returned.
 */
int32_t lseek(int32_t fd, int32_t offset, int32_t whence);

#endif /* _FS_H */

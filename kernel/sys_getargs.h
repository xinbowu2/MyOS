#ifndef _SYS_GETARGS_H
#define _SYS_GETARGS_H
#include "types.h"

/*Read the program's command line arguments into a user-level buffer*/
int32_t getargs(uint8_t* buf, int32_t nbytes);

#endif /*_SYS_GETARGS_H*/

# This file contains x86 code that helps implements system calls.

#define ASM 1
#include "syscall.h"
#include "x86_desc.h"

#define JT_ELEM_SIZE 4
#define REG_SIZE 4

.data

sch_invalid_msg: .string "Attempted to call non-existent system call %d\n"

.text

# Implements all system calls by saving all registers, picking correct syscall
# entry in syscall_jump_table, calling the implementation with the correct
# number of arguments, and then restoring registers and returning to the user.
# Note that the return value of the syscall (in %eax) is left alone so it can
# be passed to the user (even if the syscall doesn't return anything).
# Input: %eax stores the syscall number (identifies which syscall to call).
#        %ebx, %ecx, %edx, %esi, %edi, %ebp store the 6 arguments to the
#        syscall. They will be pushed to the stack before calling impl in that
#        order.
# Output: Returns whatever the syscall returns. Has same effect as chosen syscall.
.globl system_call_handler
system_call_handler:
	# Return error (-1) if not valid syscall num.
	cmpl	$NUM_SYSCALLS, %eax
	jae	sch_invalid

	# Save registers (except %eax).
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushl	%ds

	# Setup %ds for kernel space.
	pushl	$KERNEL_DS
	popl	%ds

	# Push again for passing as arguments to syscall because C standard
	# doesn't prevent modification of arguments.
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx

	call	*syscall_jump_table(, %eax, JT_ELEM_SIZE)
	# Restore stack by removing args. 6 arguments at 4 bytes each.
	addl	$MAX_SYSCALL_ARGS*REG_SIZE, %esp

	# Restore registers (except %eax).
	popl	%ds
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	iret


sch_invalid:
	pushl	%eax
	pushl	$sch_invalid_msg
	call	printf
	# 8 to restore stack.
	addl	$8, %esp

	movl	$-1, %eax
	iret

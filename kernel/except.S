# This file contains exception handlers that will be called when various x86
# exceptions happen. The handler functions here should be pointed to by entries
# in the IDT for the appropriate exception.

#define ASM 1
#include "sys_halt.h"

.data

# Divide by zero error message.
divide_by_zero_str: .string "\nDivide by zero!\n\n"

# Overflow error message.
overflow_str: .string "\nOverflow!\n\n"

# Bounds exception message.
bound_exception_str: .string "\nBounds exception!\n\n"

# Invalid opcode message.
invalid_opcode_str: .string "\nInvalid opcode!\n\n"

# FPU is not available message.
fpu_not_available_str: .string "\nFPU not available!\n\n"

# Coprocessor had segumentation exception message.
coprocessor_segment_str: .string "\nCoprocessor segmentation exception\n\n"

# Segment not present exception message.
segment_not_present_str: .string "\nSegment is not present!\n\n"

# Stack exception message.
stack_exception_str: .string "\nStack exception!\n\n"

# General protection exception message.
general_protection_str: .string "\nGeneral Protection Fault!\n\n"

# printf format string for printing page fault cause nicely.
page_fault_str: .string "\nPage Fault!!!\nAddr: 0x%x\nCause: 0x%x\nInst: 0x%x\n\n"

# Floating point error message.
floating_point_error_str: .string "\nFloating point error!\n\n"

# Alignment check exception message.
alignment_check_str: .string "\nAlignment check exception!\n\n"

# SIMD floating point error exception message.
simd_floating_point_error_str: .string "\nSIMD floating point error!\n\n"

.text

# Gets the processor stuck in an infinite halt loop. Mainly intended for end
# of entry() in kernel.c or bsod().
# Output: Current execution will never return from this function. Interrupts
#         may still happen as usual.
.globl infinite_halt
infinite_halt:
	hlt
	jmp	infinite_halt



# Processor Exceptions
# 
# Exception Types:
# 
# * Fault - the return address points to the instruction that caused the
#           exception. The exception handler may fix the problem and then
#           restart the program, making it look like nothing has happened.
# * Trap - the return address points to the instruction after the one that has
#          just completed.
# * Abort - the return address is not always reliably supplied. A program which
#           causes an abort is never meant to be continued.
# 
# Number  Description                                         Type
# ----------------------------------------------------------------------
# 0       Divide-By-Zero                                      fault
# 1       Debug Exception                                     trap or fault
# 2       Non-Maskable Interrupt (NMI)                        trap
# 3       Breakpoint (INT 3)                                  trap
# 4       Overflow (INTO with EFlags[OF] set)                 trap
# 5       Bound Exception (BOUND on out-of-bounds access)     trap
# 6       Invalid Opcode                                      trap
# 7       FPU Not Available                                   trap
# 8*      Double Fault                                        abort
# 9       Coprocessor Segment Overrun                         abort
# 10*     Invalid TSS                                         fault
# 11*     Segment Not Present                                 fault
# 12*     Stack Exception                                     fault
# 13*     General Protection                                  fault or trap
# 14*     Page fault                                          fault
# 15      Reserved
# 16      Floating-Point Error                                fault
# 17      Alignment Check                                     fault
# 18      Machine Check                                       abort
# 19-31 Reserved By Intel
# 32-255 Available for Software and Hardware Interrupts
# *These exceptions have an associated error code.

# C function exception handler of type irq_handler_func that will be called to
# handle a divide by zero exception.
.globl divide_by_zero_handler
divide_by_zero_handler:
	pushl	$divide_by_zero_str
	call	printf
	pushl	$divide_by_zero_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a debug exception.
.globl debug_exception_handler
debug_exception_handler:
	call	debug_exception_bsod_handler
	iret

# C function exception handler of type irq_handler_func that will be called to
# handle a non maskable interrupt.
.globl non_maskable_interrupt_handler
non_maskable_interrupt_handler:
	call	non_maskable_interrupt_bsod_handler
	iret

# C function exception handler of type irq_handler_func that will be called to
# handle a breakpoint.
.globl breakpoint_handler
breakpoint_handler:
	call	breakpoint_bsod_handler
	iret

# C function exception handler of type irq_handler_func that will be called to
# handle an int overflow exception.
.globl overflow_handler
overflow_handler:
	pushl	$overflow_str
	call	printf
	pushl	$overflow_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle an array bound exception.
.globl bound_exception_handler
bound_exception_handler:
	pushl	$bound_exception_str
	call	printf
	pushl	$bound_exception_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle an invalid opcode exception.
.globl invalid_opcode_handler
invalid_opcode_handler:
	pushl	$invalid_opcode_str
	call	printf
	pushl	$invalid_opcode_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a device not available exception.
.globl fpu_not_available_handler
fpu_not_available_handler:
	pushl	$fpu_not_available_str
	call	printf
	pushl	$fpu_not_available_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a double fault exception.
.globl double_fault_handler
double_fault_handler:
	call	double_fault_bsod_handler
	iret

# C function exception handler of type irq_handler_func that will be called to
# handle a coprocessor segment exception.
.globl coprocessor_segment_handler
coprocessor_segment_handler:
	pushl	$coprocessor_segment_str
	call	printf
	pushl	$coprocessor_segment_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle an invalid TSS exception.
.globl invalid_TSS_handler
invalid_TSS_handler:
	call	invalid_TSS_bsod_handler
	iret

# C function exception handler of type irq_handler_func that will be called to
# handle a segment is missing (present bit is clear).
.globl segment_not_present_handler
segment_not_present_handler:
	pushl	$segment_not_present_str
	call	printf
	pushl	$segment_not_present_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a stack exception.
.globl stack_exception_handler
stack_exception_handler:
	pushl	$stack_exception_str
	call	printf
	pushl	$stack_exception_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a general protection exception.
.globl general_protection_handler
general_protection_handler:
	pushl	$general_protection_str
	call	printf
	pushl	$general_protection_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a page fault exception.
.globl page_fault_handler
page_fault_handler:
	movl	%cr2, %eax
	pushl	%eax
	pushl	$page_fault_str
	call	printf

	pushl	infinite_halt
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a floating point error exception.
.globl floating_point_error_handler
floating_point_error_handler:
	pushl	$floating_point_error_str
	call	printf
	pushl	$floating_point_error_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle alignment check exception.
.globl alignment_check_handler
alignment_check_handler:
	pushl	$alignment_check_str
	call	printf
	pushl	$alignment_check_bsod_handler
	jmp	return_to_parent

# C function exception handler of type irq_handler_func that will be called to
# handle a machine check exception (something is wrong with the processor).
.globl machine_check_handler
machine_check_handler:
	call	machine_check_bsod_handler
	iret

# C function exception handler of type irq_handler_func that will be called to
# handle a SIMD floating point error exception.
.globl simd_floating_point_error_handler
simd_floating_point_error_handler:
	pushl	$simd_floating_point_error_str
	call	printf
	pushl	$simd_floating_point_error_bsod_handler
	jmp	return_to_parent

# Checks if user process caused exception, then halts it and returns to parent
# (if no parent, restarts the shell). If no user process, than call the
# given handler (probably BSOD).
# Input: The handler to call if there is no user process that caused the
#	 exception should be on the top of the stack.
# Output: As if user process called halt(256) or calls the given handler if no
#	  user process.
return_to_parent:
	movl	curr_pid, %eax
	cmpl	$0, %eax
	jl	rtp_call_handler

	pushl	$PROCESS_RETURN_EXCEPT
	call	internal_halt

	# Should never be reached since return to parent.
	iret

rtp_call_handler:
	call	*(%esp)

	# Ideally shouldn't be reached.
	iret

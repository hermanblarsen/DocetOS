#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include <stddef.h>

/* Describes a single stack frame, as found at the top of the stack of a task
   that is not currently running.  Registers r0-r3, r12, lr, pc and psr are stacked
	 automatically by the CPU on entry to handler mode.  Registers r4-r11 are subsequently
	 stacked by the task switcher.  That's why the order is a bit weird. */
typedef struct s_StackFrame {
	volatile uint32_t r4;
	volatile uint32_t r5;
	volatile uint32_t r6;
	volatile uint32_t r7;
	volatile uint32_t r8;
	volatile uint32_t r9;
	volatile uint32_t r10;
	volatile uint32_t r11;
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr;
	volatile uint32_t pc;
	volatile uint32_t psr;
} OS_StackFrame_t;

typedef struct {
	/* Task stack pointer.  It's important that this is the first entry in the structure,
	   so that a simple double-dereference of a TCB pointer yields a stack pointer. */
	void * volatile sp;
	/* This field is intended to describe the state of the thread - whether it's yielding,
	   runnable, or whatever.  Only one bit of this field is currently defined (see the #define
	   below), so you can use the remaining 31 bits for anything you like. */
	uint32_t volatile state;
	/* The remaining fields are provided for expandability.  None of them have a dedicated
	   purpose, but their names might imply a use.  Feel free to use these fields for anything
	   you like. */
	uint32_t volatile priority;
	uint32_t volatile data;
} OS_TCB_t;

/* Constants that define bits in a thread's 'state' field. */
#define TASK_STATE_YIELD    (1UL << 0) // Bit zero is the 'yield' flag

#endif /* _TASK_H_ */

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include <stddef.h>


/*=============================================================================
**       Type Definitions
=============================================================================*/
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

typedef struct OS_TCB_t {
	/* Task stack pointer.  It's important that this is the first entry in the structure,
	   so that a simple double-dereference of a TCB pointer yields a stack pointer. */
	void * volatile sp;
	/*  This field is intended to describe the state of the task - see bit definitions
         below (TASK_STATE_*). The remaining bits can be utilised by the user.
        */
	uint32_t volatile state;
	/* This field holds the task priority  */
	uint32_t volatile priority;
    /* This field is used to store any data to aid the OS oepration and flow,
		including awakening times for sleeping tasks. */
	uint32_t volatile data;
    /* Holds the previous task when in a runnable state,
		implementing a doubly-linked list.  */
    struct OS_TCB_t * volatile prev;
	/* Holds the next task when in a runnable state,
		implementing a doubly-linked list. Also used in other places in the
		OS, including to implement a singly-linked list in the resource wait queue*/
    struct OS_TCB_t * volatile next;
} OS_TCB_t;


/*=============================================================================
**       Definitions
=============================================================================*/
/* Constants that define bits in a thread's 'state' field. */
#define TASK_STATE_YIELD    (1UL << 0) // Bit zero is the 'yield' flag 
#define TASK_STATE_SLEEP    (1UL << 1) // Bit one is the 'sleep' flag 
#define TASK_STATE_WAIT     (1UL << 2) // Bit two is the 'wait' flag
#define TASK_STATE_PRIORITY_INHERITED    (1UL << 3) //Bit five is whether or not the task is currently running with inherited priority

#endif /* _TASK_H_ */

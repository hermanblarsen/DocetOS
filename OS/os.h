#ifndef _OS_H_
#define _OS_H_

#include "task.h"

/*=============================================================================
 *  This is the main file of DocetOS.
===============================================================================
**       Example Use of OS
*******************************************************************************
#include "everything.h"
//TODO
=============================================================================*/


/*=============================================================================
**       Type Definitions
=============================================================================*/
/* A set of numeric constants giving the appropriate SVC numbers for various callbacks.
   If this list doesn't match the SVC dispatch table in os_asm.s, BIG TROUBLE will ensue. */
enum OS_SVC_e {
	OS_SVC_ENABLE_SYSTICK=0x00,
	OS_SVC_SCHEDULE,
    OS_SVC_ADD_TASK,
	OS_SVC_EXIT_TASK,
	OS_SVC_YIELD_TASK,
    OS_SVC_REMOVE_TASK,
    OS_SVC_WAIT,
    OS_SVC_NOTIFY
};

/* A structure to hold callbacks for a scheduler, plus a 'preemptive' flag */
typedef struct {
	uint_fast8_t preemptive;
	OS_TCB_t const * (* scheduler_callback)(void);
	void (* taskAdd_callback)(OS_TCB_t * const new_task);
	void (* taskExit_callback)(OS_TCB_t * const finished_task);
    void (* taskRemove_callback)(OS_TCB_t * const sleep_wait_task);
    void (* wait_callback)(void * const reason, void * const resource_wait_queue_head, uint32_t fail_fast_counter);
    void (* notify_callback)(void * const resource_wait_queue_head);
} OS_Scheduler_t;

/*=============================================================================
**       Global Idle TCB Declaration
=============================================================================*/
extern OS_TCB_t const * const OS_idleTCB_p;


/*=============================================================================
**       OS management functions
=============================================================================*/
/**
 * [OS_init Initialises the OS.  Must be called before OS_start().]
 * @param scheduler [pointe to a OS_Scheduler_t to initialise the OS with]
 */
void OS_init(OS_Scheduler_t const * scheduler);

/**
 * [OS_start Starts the OS kernel.  Never returns.
 	OS must be initialised with OS_init before this call.]
 */
void OS_start(void);

/**
 * [OS_currentTCB Returns a pointer to the TCB of the currently running task.]
 * @return  [pointe to current OS_TCB_t]
 */
OS_TCB_t * OS_currentTCB(void);

/**
 * [OS_elapsedTicks Returns the number of elapsed systicks since the last reboot (modulo 2^32).]
 * @return elapsed_ticks [elapsed ticks since reboot (uint32_t)]
 */
uint32_t OS_elapsedTicks(void);

/**
 * [OS_currentFastFailCounter  Returns the Fail-Fast Counter used to guard against
 *   deadlock situations from _OS_wait() calls. The counter is incremented from
 *   _OS_notify]
 * @return fail_fast_counter [current fast fail counter value (uint32_t).]
 */
uint32_t OS_currentFastFailCounter (void);


/*=============================================================================
**       Task creation and management functions
=============================================================================*/
/**
* [OS_initialiseTCB Initialises a task control block (TCB) and its associated stack.
*  The stack is prepared with a frame such that when this TCB is first used
*   in a context switch, the given function will be executed.  If and when the
*   function exits, a SVC call will be issued to kill the task, and a callback
    will be executed]
* @param TCB   [pointer to a OS_TCB_t to initialise]
* @param stack [pointer to the TOP OF a region of memory to be used as a
* 	stack (stacks are full descending).
   NOTE that the stack MUST be 8-byte aligned.  This means if (for example)
    malloc() is used to create a stack, the result must be checked for alignment,
	and then the stack size must be added to the pointer for passing to this function]
* @param func  [pointer to the function that the task should execute]
* @param priority [The priority to assign to the TCB.
* 	Must be 0<priority<=PRIORITY_MAX]
* @param data 	[void pointer to data that the task should receive]
*/
void OS_initialiseTCB(OS_TCB_t * TCB, uint32_t * const stack, void (* const func)(void const * const),
    uint32_t priority, void const * const data);

/**
 * [OS_addTask SVC delegate to add a task]
 * @param OS_SVC_ADD_TASK [pointer to the OS_TCB_t to be added to the scheduler]
 */
void __svc(OS_SVC_ADD_TASK) OS_addTask(OS_TCB_t const * const);


/*=============================================================================
**       Scheduling functions
=============================================================================*/
/**
 * [OS_yield SVC delegate to yield the current task. This will yield to other
 * 	tasks of the same (or higher) priority, but if this is the highest priority
 * 	task it will simply run again.]
 */
void __svc(OS_SVC_YIELD_TASK) OS_yield(void);

#endif /* _OS_H_ */

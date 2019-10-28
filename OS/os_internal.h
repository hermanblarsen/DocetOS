#ifndef _OS_INTERNAL_H_
#define _OS_INTERNAL_H_

#include "os.h"
#include "task.h"
#include "stm32f4xx.h"

/*=============================================================================
 *      TODOCOMMENT
=============================================================================*/

/*=============================================================================
**       Type Definitions
=============================================================================*/
/*  Describes the available SVC stack frame as found at the top of the active
     stack on entry to an interrupt, as registers r0-r3, r12, lr, pc and psr
     are stacked automatically by the CPU on entry to handler mode. */
typedef struct {
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	const volatile uint32_t r12;
	const volatile uint32_t lr;
	const volatile uint32_t pc;
	const volatile uint32_t psr;
} _OS_SVC_StackFrame_t;


/*=============================================================================
**       Global Variable Declarations
=============================================================================*/
extern OS_TCB_t * volatile _currentTCB;


/*=============================================================================
**       Internal Scheduling Functions
=============================================================================*/
/*****************************************************************************
**  SVC Delegates
******************************************************************************/
/**
 * [_OS_wait SVC delegate to let the current task enter a wait state when it has tried
     to aquire an unavailable resource]
 * @param resource [the resource to wait for]
 * @param resource_wait_queue_head  [pointer to the head (OS_TCB_t *) of the wait queue]
 * @param fail_fast_counter  [fail fast count from when wait was called]
 */
void __svc(OS_SVC_WAIT) _OS_wait(void *, void *, const uint32_t);

/**
 * [_OS_notify SVC delegate to notify waiting tasks that the resource they are
 * 	waiting for is now available.]
 * @param resource_wait_queue_head  [pointer to the head (OS_TCB_t *) of the wait queue]
 */
void __svc(OS_SVC_NOTIFY) _OS_notify(void *);

/**
 * [_OS_taskExit SVC delegate to exit a finished task]
 */
void __svc(OS_SVC_EXIT_TASK) _OS_taskExit(void);

/*  */
/**
 * [_OS_removeTask SVC delegate to remove a task to be put into wait or sleep queues]
 * @param tcb [pointer to OS_TCB_t to remove]
 */
void __svc(OS_SVC_REMOVE_TASK) _OS_removeTask(OS_TCB_t const * const);

/*****************************************************************************
**  C  Function Prototypes
******************************************************************************/
/**
 * [_OS_taskEnd Function automatically called if the task exits by writing
 * 	its address to the link register on initialisation]
 */
void _OS_taskEnd(void);

/*****************************************************************************
**  ASM Function Prototypes (os_asm.c)
******************************************************************************/
/**
 * [_task_switch Function to handle task switches]
 */
void _task_switch(void);

/**
 * [_task_initialiseSwitch Initialises the system]
 * @param idleTask [the system idle task]
 */
void _task_initialiseSwitch(OS_TCB_t const * const idleTask);

#endif /* _OS_INTERNAL_H_ */

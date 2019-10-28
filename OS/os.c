#include "os.h"
#include "os_internal.h"
#include "stm32f4xx.h"
#include "roundRobin.h"
#include "mutex.h"
#include "sleep.h"
#include <stdlib.h>
#include <string.h>
#include "debug.h"


/*=============================================================================
**      Static Variables
=============================================================================*/
__align(8)
/* Idle task stack frame area and TCB.  The TCB is not declared const, to ensure that it is placed in writable
   memory by the compiler.  The pointer to the TCB _is_ declared const, as it is visible externally - but it will
   still be writable by the assembly-language context switch. */
static OS_StackFrame_t const volatile _idleTaskSF;
static OS_TCB_t OS_idleTCB = { (void *)(&_idleTaskSF + 1), 0, 0, 0 };
OS_TCB_t const * const OS_idleTCB_p = &OS_idleTCB;

/* If in DEBUG_HARD mode define these as NON-static to be able to add to watches from outside this translation unit*/
#ifndef DEBUG_HARD 
    /* Total elapsed ticks, will overflow about every 49.71 days ((2^32 -1) / (1000 * 3600 *24)) */
    static volatile uint32_t _ticks = 0;
    /* Fast-Fail Check Counter to prevent deadlock at failed mutex aquisition when OS wait is called */
    static volatile uint32_t _fast_fail_counter = 0; 
#else
    volatile uint32_t _ticks = 0;
    volatile uint32_t _fast_fail_counter = 0;    
#endif

/* Pointer to the 'scheduler' struct containing callback pointers */
static OS_Scheduler_t const * _scheduler = 0;

/*=============================================================================
**      Global Internal Variable
=============================================================================*/
/* GLOBAL: Holds pointer to current TCB.  DO NOT MODIFY, EVER. */
OS_TCB_t * volatile _currentTCB = 0;


/*=============================================================================
**      Functions
=============================================================================*/
/* Getter for the current TCB pointer.  Safer to use because it can't be used
   to change the pointer itself. */
OS_TCB_t * OS_currentTCB(void) {
	return _currentTCB;
}

/* Getter for the current time in ticks, set to be incremented every 1 ms. */
uint32_t OS_elapsedTicks(void) {
	return _ticks;
}

/* Getter for the Fast-Fail Check Counter, incremented with task notifications and
    utilised to prevent race-conditions when tasks are set to wait 
    (fail fast behaviour). */
uint32_t OS_currentFastFailCounter(void) {
	return _fast_fail_counter;
}

/* IRQ handler for the system tick.  Schedules PendSV */
void SysTick_Handler(void) {
	_ticks = _ticks + 1;  
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* Sets up the OS by storing a pointer to the structure containing all the callbacks.
   Also establishes the system tick timer and interrupt if preemption is enabled. */
void OS_init(OS_Scheduler_t const * scheduler) {
	_scheduler = scheduler;
    *((uint32_t volatile *)0xE000ED14) |= (1 << 9); // Set STKALIGN
	ASSERT_DEBUG(_scheduler->scheduler_callback);
	ASSERT_DEBUG(_scheduler->taskAdd_callback);
 	ASSERT_DEBUG(_scheduler->taskExit_callback);
    ASSERT_DEBUG(_scheduler->taskRemove_callback);
    ASSERT_DEBUG(_scheduler->wait_callback);
    ASSERT_DEBUG(_scheduler->notify_callback);
}

/* Starts the OS and never returns. */
void OS_start(void) {
	ASSERT_DEBUG(_scheduler);
	// This call never returns (and enables interrupts and resets the stack)
	_task_initialiseSwitch(OS_idleTCB_p);
}

/* Initialises a task control block (TCB) and its associated stack.  See os.h for details. */
void OS_initialiseTCB(OS_TCB_t * TCB, uint32_t * const stack, void (* const func)(void const * const), uint32_t priority, void const * const data) {
	TCB->sp = stack - (sizeof(OS_StackFrame_t) / sizeof(uint32_t));
    /* Make sure priority is within bounds. User will not be notified, but will cause adverse problems
        if designed to work according to priorities that have been modified.
        Checking for pri>MAX is sufficient due to unsigned. */
    if (priority >= PRIORITY_MAX) {
        ASSERT_DEBUG(0);
        priority = PRIORITY_MAX;
    }   
    TCB->priority = priority; 
    TCB->state = TCB->data = 0;
    TCB->next = TCB->prev = NULL;
	OS_StackFrame_t *sf = (OS_StackFrame_t *)(TCB->sp);
	memset(sf, 0, sizeof(OS_StackFrame_t));
	/* By placing the address of the task function in pc, and the address of _OS_taskEnd() in lr, the task
	   function will be executed on the first context switch, and if it ever exits, _OS_taskEnd() will be
	   called automatically */
	sf->lr = (uint32_t)_OS_taskEnd;
	sf->pc = (uint32_t)(func);
	sf->r0 = (uint32_t)(data);
	sf->psr = 0x01000000;  /* Sets the thumb bit to avoid a big steaming fault */
}

/*=============================================================================
**      SVC Handlers
=============================================================================*/
/* Function that's called by a task when it ends (the address of this function is
   inserted into the link register of the initial stack frame for a task).  Invokes a SVC
   call (see os_internal.h); the handler is _svc_OS_taskExit() (see below). */
void _OS_taskEnd(void) {
	_OS_taskExit();
	/* DO NOT STEP OUT OF THIS FUNCTION when debugging!  PendSV must be allowed to run
	   and switch tasks.  A hard fault awaits if you ignore this.  */
}

/* SVC handler to enable systick from unprivileged code */
void _svc_OS_enableSystick(void) {
	if (_scheduler->preemptive) {
		SystemCoreClockUpdate();
		SysTick_Config(SystemCoreClock / 1000);
		NVIC_SetPriority(SysTick_IRQn, 0x10);
	}
}

/* SVC handler for OS_schedule().  Simply schedules PendSV */
void _svc_OS_schedule(void) {
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler to invoke the scheduler (via a callback) from PendSV */
OS_TCB_t const * _OS_scheduler(void) {
	return _scheduler->scheduler_callback();
}

/* SVC handler to add a task.  Invokes a scheduler callback. */
void _svc_OS_taskAdd(_OS_SVC_StackFrame_t const * const stack) {
	/* The TCB pointer is on the stack in the r0 position, having been passed as an
	   argument to the SVC pseudo-function.  SVC handlers are called with the stack
	   pointer in r0 (see os_asm.s) so the stack can be interrogated to find the TCB
	   pointer. */
	_scheduler->taskAdd_callback((OS_TCB_t *)stack->r0);
}

/* SVC handler that's called by _OS_taskEnd() when a task finishes.  Invokes the
   task end callback and then queues PendSV to call the scheduler. */
void _svc_OS_taskExit(void) {
	_scheduler->taskExit_callback(_currentTCB);
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler for OS_yield().  Sets the TASK_STATE_YIELD flag and schedules PendSV */
void _svc_OS_taskYield(void) {
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler to remove a task.  Invokes a scheduler callback. */
void _svc_OS_taskRemove(_OS_SVC_StackFrame_t const * const stack) {
	_scheduler->taskRemove_callback((OS_TCB_t *)stack->r0);
    //Schedule a task change after removing the task from the scheduler.
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/* SVC handler for _OS_wait(). Simply calls the scheduler wait function with the unit32_t* reason (* mutex) as argument*/
void _svc_OS_taskWait(_OS_SVC_StackFrame_t const * const stack) {
    
    /* Call the Scheduler Wait callback with arguments
                r0 (OS_Mutex_t * OR OS_Semaphore_t *)
                r1 (OS_TCB_t ** head_of_resource_wait_queue)
                r2 (uint32_t fail_fast_counter)  */
    _scheduler->wait_callback((OS_Mutex_t *)stack->r0, (OS_TCB_t **)stack->r1, (uint32_t)stack->r2);
    
}

/* SVC handler for _OS_notify().  Simply calls the scheduler notify function with the uint32_t* reason as argument.
	Will increment the _fast_Fail_counter for the ability to check for deadlock situations prior to _OS_wait() */
void _svc_OS_taskNotify(_OS_SVC_StackFrame_t const * const stack) {
    _fast_fail_counter++;
    __CLREX();
    /* Call the Scheduler Notify callback with arguments r1 (TCB pointer) */
    _scheduler->notify_callback((OS_TCB_t **)stack->r0);
}


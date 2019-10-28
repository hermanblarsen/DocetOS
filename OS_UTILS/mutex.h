#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <stdint.h>
#include "task.h"

/*=============================================================================
 *  This file contains tools for Mutual Exlusion (MutEx) supported by the OS.
===============================================================================
**       Example Use
*******************************************************************************
#include "mutex.h"
//TODO
=============================================================================*/


/*=============================================================================
**       Type Definitions
=============================================================================*/
/* A structure to hold the mutex owner, recursive counter, and a pointer
    to the head of a singly linked list of queued tasks waiting for the mutex*/
typedef struct {
    /* Pointer to the task that owns of the mutex, or 0 if available. */
	OS_TCB_t * volatile tcb;
    /* Counter to allow for recursive access of mutex from within the same task */
    uint32_t volatile counter;
    /* Pointer to the first task waiting for this mutex to become available,
    or 0 if there are no waiting tasks. */
	OS_TCB_t * volatile wait_queue_head;
} OS_Mutex_t;


/*=============================================================================
**       Function Prototypes
=============================================================================*/
/**
 * [OS_mutexInitialise  Initialises a recursive mutex, where the same task
 * 	 can acquire the mutex several times. Balanced acquire and release function
 * 	 calls are expected.]
 * @param mutex [pointer to the OS_Mutex_t to be initialised]
 */
void OS_mutexInitialise (OS_Mutex_t * mutex);

/**
 * [OS_mutexAcquire Aquires the mutex if it is not acquired already,
 *   or waits until it is released if it was not.
 * 	If the mutex is already acquired, this will wait until it is released.
 *  If the mutex is never released elsewhere, this function will never return.
 *  Once this function returns, the mutex has been successfully (re)taken.
 *  CIMSIS compiler-specific primitives for LDREX and STREX are used within]
 * @param mutex [pointer to the OS_Mutex_t to be acquired]
 */
void OS_mutexAcquire(OS_Mutex_t * mutex);

/**
 * [OS_mutexRelease Releases the mutex if the task that calls this holds
 * 	 the mutex, and the recursive count is 0. If the recursive count > 0,
 * 	 the mutex will not notify waiting tasks.
 * 	When this function returns, the mutex' recursive count has been
 * 	 decremented, and depending if the call was the final call of a balanced
 * 	 set of acquire/release calls (count = 0) the mutex will be released and
 * 	 waiting tasks to be notified of its availability.
 *  CIMSIS compiler-specific primitives for LDREX and STREX are used within]
 * @param mutex [pointer to the OS_Mutex_t to be released]
 */
void OS_mutexRelease(OS_Mutex_t * mutex);

#endif /* _MUTEX_H_ */

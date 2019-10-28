#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <stdint.h>
#include "task.h"

/*=============================================================================
 *  This file contains tools for task synchronisation (semaphores) supported
 *   by the OS.
===============================================================================
**       Example Use
*******************************************************************************
#include "semaphore.h"
//TODO
=============================================================================*/


/*=============================================================================
**       Type Definitions
=============================================================================*/
/*  A structure to hold the available semaphore tokens, the size of
	 the semaphore, and and a pointer to the head of a singly linked list
     of queued tasks waiting to give/tokens. */
typedef struct {
    /* The current number of tokens held by the semaphore */
    uint32_t volatile tokens;
    /* The max number of tokens held by the semaphore */
    uint32_t volatile max_tokens;
    /* Pointer to the first task waiting for this semaphore to become available,
    or 0 if there are no waiting tasks. */
	OS_TCB_t * volatile wait_queue_head;
} OS_Semaphore_t;


/*=============================================================================
**       Function Prototypes
=============================================================================*/
/**
* [OS_semaphoreInitialise  Initialises a counting semaphore. It will block tasks
*   when either empty and full, apart from when size = 0 where it will
*   only block task when empty.]
 * @param semaphore   [pointer to the OS_Semaphore_t to initialise]
 * @param size        [size of the semaphore. This can either be:
 *   size > 1 - will block tasks trying to take when empty and give when full
 *   size = 0 - will block tasks trying to take when empty, but has no upper bound]
 * @param init_tokens [the number of tokens to initialise the semaphore with.
    Must be 0 < init_tokens <= size]
 */
void OS_semaphoreInitialise(OS_Semaphore_t * semaphore, const uint32_t size, const uint32_t init_tokens);

/**
 * [OS_semaphoreInitialiseBinary Initialises a binary semaphore.
 *  It will block waiting tasks both when empty(0) and full(1), and is a special
 *   case of OS_semaphoreInitialise with size=1.]
 * @param semaphore [pointer to the OS_Semaphore_t to initialise]
 * @param init_full [the number of tokens to initialise with.
 *  Must be either 1 or 0s]
 */
void OS_semaphoreInitialiseBinary(OS_Semaphore_t * semaphore, const uint32_t init_full);

/**
 * [OS_semaphoreInitialiseCounting Initialises a counting semaphore with size
 *   and starting tokens equal to 0, a special case of OS_semaphoreInitialise.
 *  This semaphore has no ceiling, and can be given (2^32)-1 tokens before.
 *   overflowing. The overflow is up to the user to prevent if relying on
 *   an equal number of gives and takes, but will not effect OS_semaphoreGive
 *   from continueing after the overflow.]
 * @param semaphore [pointer to the OS_Semaphore_t to initialise]
 */
void OS_semaphoreInitialiseCounting(OS_Semaphore_t * semaphore);

/**
 * [OS_semaphoreTake Takes a semaphore token if available.
 *  If a token is not  available, this will wait until it is made available.
 *  If a token is never returned elsewhere, this function will never return.
 *  Once this function returns, a token has been successfully taken.
 *  CIMSIS compiler-specific primitives for LDREX and STREX are used within.]
 * @param semaphore [pointer to the OS_Semaphore_t to take a token from]
 */
void OS_semaphoreTake(OS_Semaphore_t * semaphore);

/**
 * [OS_semaphoreGive Gives a semaphore token if possible (semaphore not full).
 *  If semaphore is full, this will wait until a token is taken.
 *  If a token is never taken elsewhere, this function will never return.
 *  Once this function returns, a token has been successfully given.
 *  CIMSIS compiler-specific primitives for LDREX and STREX are used within]
 * @param semaphore [pointer to the OS_Semaphore_t to give a token to]
 */
void OS_semaphoreGive(OS_Semaphore_t * semaphore);

#endif /* _SEMAPHORE_H_ */

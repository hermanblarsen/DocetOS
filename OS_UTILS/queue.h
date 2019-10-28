#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>
#include "mutex.h"
#include "semaphore.h"

/*=============================================================================
 *  This file adds inter-task communication to the OS, and can be used
 *   to exchange data between tasks as long as each task has a reference to the
 *   queue and knowledge about what goes in it.
 *  The queue can technically store arbitrarily sized values, given at initialisation,
 *   but should for performance only be used for copying of very small structure,
 *   otherwise pointers to structures will be much more efficient.
===============================================================================
**       Example Use
*******************************************************************************
#include "queue.h"
//TODO
=============================================================================*/


/*=============================================================================
**       Type Definitions
=============================================================================*/
/* A structure containing the necessary variables for the queue implemented as
    a ring/circular buffer, including and all required mutexes and semaphores
    for protection against corruption (mutex preventing simultaneous access) as
    well as overfilling and exhaustion of the queue (semaphores for access
    to write and read) */
typedef struct  {
    uint32_t length, item_size;
    uint8_t * start, * end, * volatile head, * volatile tail;
    OS_Mutex_t mutex_rw;
    OS_Semaphore_t sem_r, sem_w;
} OS_Queue_t;


/*=============================================================================
**       Function Prototypes
=============================================================================*/
/**
 * [OS_queueInitialise Initialises the queue. Must be done
 *  prior to starting the OS.]
 * @param queue           [pointer to the OS_Queue_t to initialise]
 * @param static_memory   [pointer to statically declared memory to use with
 *   the queue. This must fit [queue_length*queue_item_size] bytes, where a
 *   recommended use case can be found at the top of this header file.
 *  For internal processes to execute safely, this memory must be at least word
 *   aligned (4 bytes).
 *  This static memory MUST be a valid writable and readable memory address,
 *   and the queue will not operate correctly/at all if it isn't.]
 * @param queue_length    [the size of the queue (number of elements it can hold)]
 * @param queue_item_size [the size in bytes of one element. All elements
 *   in the queue must be of the same type.]
 */
void OS_queueInitialise(OS_Queue_t * queue, void * const static_memory, const uint32_t queue_length, const uint32_t queue_item_size);

/**
 * [OS_queueEnqueue Enqueue an item to the back of the queue if there is
 *   sufficient space, or wait until there is space to enqueue the item.
 *  If the queue is full and no elements ever get removed,
 *   this function will never return.]
 * @param queue [pointer to the OS_Queue_t to enqueue an item to]
 * @param item  [pointer to desired item to enqueue]
 */
void OS_queueEnqueue(OS_Queue_t * queue, const void * item);

/**
 * [OS_queueDequeue Dequeue an item from the front of the queue if it is
 *   not empty, or wait until there is an element to dequeue.
 *  If there no elements are in (or get added to) the queue, this function will
 *    never return.]
 * @param queue       [pointer to the OS_Queue_t to dequeue an item from]
 * @param item_buffer [pointer to desired item_buffer to dequeue to]
 */
void OS_queueDequeue(OS_Queue_t * queue, void * item_buffer);

#endif /* _QUEUE_H_ */

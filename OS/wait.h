#ifndef _WAIT_H
#define _WAIT_H

#include "task.h"

/*=============================================================================
 *      Function Prototypes for Internal OS Operation.
 *      Used by wait.c and semaphore.c
=============================================================================*/
/**
 * [wait_queueInsert Inserts a task into the wait queue of an unavailable
 *  resource (mutex or semaphore).]
 * @param tcb_wait_queue_head [double pointer to a OS_TCB_t, the head of
 *   the resource's wait queue]
 * @param tcb                 [pointer to the OS_TCB_t to be added to the queue]
 */
void wait_queueInsert(OS_TCB_t ** tcb_wait_queue_head, OS_TCB_t * tcb);

/**
 * [wait_queueExtract Extracts a task from the wait queue of a now available
 *  resource (mutex or semaphore).]
 * @param  tcb_wait_queue_head [double pointer to a OS_TCB_t, the head of
 *   the resource's wait queue]
 * @return                     [pointer to the first OS_TCB_t of the highest
 *   priority which was added to the queue, which is removed from the queue]
 */
OS_TCB_t * wait_queueExtract(OS_TCB_t ** tcb_wait_queue_head);

#endif /* _WAIT_H */

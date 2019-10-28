#include "wait.h"
#include "debug.h"

/**
 *  This file is adding wait functionality to the OS, utilised indirectly by
 *   mutex.c and semaphore.c through _OS_wait().
 *  This file specifically implements the Singly Linked Insertion Sorted list
 *   used to maintain wait queue, of which each resource (semaphore or mutex)
 *   has a reference to the head of.
 *  The IS SLL sorts waiting tasks based on priority and is implemented on a
 *   first-come first-served basis.
 *  If the OS scale is expanded, the sorting of waiting tasks would have to be
 *   modified as it scales with the nuber of waiting tasks as O(n).
 *  IMPORTANT: The next field of the TCB will be modified within this module.
 */

/*=============================================================================
**      Functions
=============================================================================*/
/**
 * [wait_queueInsert Inserts a task into the wait queue of an unavailable
 *   resource (mutex or semaphore).
 *  Inserts the passed TCB into the wait list, sorted by priority and on a
 *   first-come first-served basis. ]
 * @param tcb_wait_queue_head [double pointer to a OS_TCB_t, the head of
 *   the resource's wait queue]
 * @param tcb                 [pointer to the OS_TCB_t to be added to the queue]
 */
void wait_queueInsert(OS_TCB_t ** volatile tcb_wait_queue_head, OS_TCB_t * tcb) {
    /* Make sure that running tasks are not mistaken for waiting tasks in the
        singly linked list. */
    tcb->next = 0;

    /* Check if the mutex have any waiting tasks. If not, add the TCB and
        return*/
    if (*tcb_wait_queue_head == 0) {
        *tcb_wait_queue_head = tcb;
        return;
    }

    /*  If we are here the mutex has at least 1 waiting task. Make a local
        pointer to the queued task head so we can traverse the list */
    OS_TCB_t * tcb_queued = *tcb_wait_queue_head;

    /*  Traverse the list, and insert the task based on priority and
         first-come first-served basis */
    if ( tcb->priority > tcb_queued->priority ) {
        /* The inserted task has higher priority than the head - insert first */
        tcb->next = tcb_queued;
        *tcb_wait_queue_head = tcb;
    } else {
        /*   While the traversed task priority is bigger than or equal to the
             task to be inserted, move to the next waiting task. */
		while ( tcb_queued->next != 0 && tcb->priority >= (tcb_queued->next)->priority ) {
            tcb_queued = tcb_queued->next;
        }
        /*  Insert the task after the traversed task of equal priority and before
             the next task of lower priority (or end of list).
            IMPORTANT: ORDER DEPENDENT */
        tcb->next = tcb_queued->next;
		tcb_queued->next = tcb;
    }
}


 /**
  * [wait_queueExtract Extracts a task from the wait queue of a now available
  *   resource (mutex or semaphore), and updates the mutex->wait_queue_head to
  *   the next waiting task.]
  * @param  tcb_wait_queue_head [double pointer to a OS_TCB_t, the head of
  *   the resource's wait queue]
  * @return                     [pointer to the first OS_TCB_t of the highest
  *   priority which was added to the queue, which is removed from the queue.
  *  IMPORTANT: The caller of this function must verify the return value:
  *   It will be 0 if there were no queued tasks.]
  */
OS_TCB_t * wait_queueExtract(OS_TCB_t ** volatile tcb_wait_queue_head) {
    /* Make a local pointer to the head of the queue which we want to return */
    OS_TCB_t * extracted_tcb = *tcb_wait_queue_head;

    /*  Update the wait list head as long as the current head is not 0, as this
         ensures that the queue is not corrupted by 'false'/incorrect 'next'
         data when the TCB address is 0.
        If updating, update to the next in queue, which will either be a
         valid TCB address or 0  */
    if ( extracted_tcb != 0 ) {
        *tcb_wait_queue_head = (*tcb_wait_queue_head)->next;
    }

    return  extracted_tcb;
}

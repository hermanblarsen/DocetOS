#include "queue.h"
#include <string.h>
#include "debug.h"

/*  This file is adding Inter Task Communication functionality to the OS via
    a simple circular buffer implementation protected by mutexes and 2 semaphores
    to eliminate any race conditions. */
/*=============================================================================
**      Functions
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
void OS_queueInitialise(OS_Queue_t * queue, void * const static_memory, const uint32_t queue_length, const uint32_t queue_item_size) {
    queue->length = queue_length;
    queue->item_size = queue_item_size;
    /*  Simplistic check for whether the supplied memory location is valid.
        This will only breakpoint if in a DEBUG mode. s*/
    ASSERT_DEBUG(static_memory);
    queue->start = (uint8_t * )static_memory;
    queue->end = queue->start + ( (queue->length * queue->item_size) - 1); //this points to the last byte in the given static memory
    queue->head = queue->tail = queue->start;

    OS_mutexInitialise( &queue->mutex_rw );
    OS_semaphoreInitialise( &queue->sem_r, queue->length, 0 );
    OS_semaphoreInitialise( &queue->sem_w, queue->length, queue->length );
}

/**
 * [OS_queueEnqueue Enqueue an item to the back of the queue if there is
 *   sufficient space, or wait until there is space to enqueue the item.
 *  If the queue is full and no elements ever get removed,
 *   this function will never return.]
 * @param queue [pointer to the OS_Queue_t to enqueue an item to]
 * @param item  [pointer to desired item to enqueue]
 */
void OS_queueEnqueue(OS_Queue_t * queue, const void * const potentially_unaligned_item) {
    /* Take a token to make sure there is awailable space to write to the queue,
        the once taken, acquire the mutex for full write access to the queue */
    OS_semaphoreTake(&queue->sem_w);
    OS_mutexAcquire(&queue->mutex_rw);

    /* Casting to single byte pointer for safer operation of potentially unaligned
        items in exchange for performance, see
        http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka3934.html*/
    uint8_t * single_byte_type_ptr = (uint8_t *)potentially_unaligned_item;
    memcpy((void *)queue->head, (void *)single_byte_type_ptr, (size_t)queue->item_size);

    /* Use uint8_t head to navigate N bytes along the queue, and make sure we
        stay within the memory start and end addresses */
    queue->head += queue->item_size;
    if (queue->head >= queue->end) {
        queue->head = queue->start;
    }

    /*  First give the semaphore, then release the mutex. It is done in this order
        to prioritise potential tasks waiting for the mutex over potential tasks
        waiting for the semaphore, apart from the statistically few cases where
        the scheduler performs a context switch in between these two lines, where
        the opposite effect will be seen. This prioritation is due to how notified
        waiting tasks are inserted into the next running task. */
    OS_semaphoreGive(&queue->sem_r);
    OS_mutexRelease(&queue->mutex_rw);
}


/**
 * [OS_queueDequeue Dequeue an item from the front of the queue if it is
 *   not empty, or wait until there is an element to dequeue.
 *  If there no elements are in (or get added to) the queue, this function will
 *    never return.]
 * @param queue       [pointer to the OS_Queue_t to dequeue an item from]
 * @param item_buffer [pointer to desired item_buffer to dequeue to]
 */
void OS_queueDequeue(OS_Queue_t * queue, void * potentially_unaligned_item_buffer) {
    /* Take a token to make sure there is awailable elements to read from the queue,
        the once taken, acquire the mutex for full read access to the queue */
    OS_semaphoreTake(&queue->sem_r);
    OS_mutexAcquire(&queue->mutex_rw);

    /* Casting to single byte pointer for safer operation of potentially unaligned
        items in exchange for performance, see
        http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka3934.html*/
    uint8_t * single_byte_type_ptr = (uint8_t *)potentially_unaligned_item_buffer;
    memcpy((void *)single_byte_type_ptr, (void *)queue->tail, (size_t)queue->item_size);

    /* Use uint8_t teail to navigate N bytes along the queue, and make sure we
        stay within the memory start and end addresses */
    queue->tail += queue->item_size;
    if (queue->tail >= queue->end) {
        queue->tail = queue->start;
    }

    /*  First give the semaphore, then release the mutex. It is done in this order
        to prioritise potential tasks waiting for the mutex over potential tasks
        waiting for the semaphore, apart from the statistically few cases where
        the scheduler performs a context switch in between these two lines, where
        the opposite effect will be seen. This prioritation is due to how notified
        waiting tasks are inserted into the next running task. */
    OS_semaphoreGive(&queue->sem_w);
    OS_mutexRelease(&queue->mutex_rw);
}

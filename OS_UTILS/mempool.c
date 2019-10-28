#include "mempool.h"

/*  This file is adding Memory Pool functionality to the OS, where the
	 user can utilise these as a means of static malloc for a embedded system
     with predetermined structures /structure sizes. */

/*=============================================================================
**      Static Function Prototypes
=============================================================================*/
/*   */
/**
 * [memPool_add Adds or deallocates to the pool, but without mutex or semaphore
 *   protection. SHould only be used in initialisation and only from main().]
 * @param  memory_pool [pointer to the OS_MemPool_t to deallocate to]
 * @return             [pointer to the allocated block of memory]
 */
static void memPool_add(OS_MemPool_t * memory_pool, void * const item);


/*=============================================================================
**      Functions
=============================================================================*/
/**
 * [OS_memPoolInitialise Initialise a memory pool]
 * @param memory_pool      [pointer to the OS_MemPool_t to initialise]
 * @param static_memory    [pointer to statically declared memory to use with
 *   the memory pool. This must fit [number_of_blocks*block_size] bytes, where a
 *   recommended use case can be found at the top of this header file.
 *  This static memory MUST be a valid writable and readable memory address,
 *   and the queue will not operate correctly/at all if it isn't
 *  Alternatively, NULL can be used if the memory pool should be
 *   as empty, and can later be added to using OS_memPoolDeallocate.]
 * @param number_of_blocks [number of blocks to be added to the memory pool]
 * @param block_size       [size in bytes of each block]
 */
void OS_memPoolInitialise(OS_MemPool_t * memory_pool, void * const static_memory, const uint32_t number_of_blocks, const uint32_t block_size) {
    /* Initialise the pool and its protection mechanisms against simultaneous access and depletion. */
    memory_pool->head = 0;
    OS_mutexInitialise(&memory_pool->mutex_rw);

    /*  Simplistic check for if the provided memory is a valid address.
        This can also be used to initialise an empty memory pool by
         intentionally supplying "(void *)0" as the static_memory and
         later to populate the pool. */
    if (static_memory != 0) {
        /*  Instantiate the semaphore with the full amount of tokens, and
             internally add the blocks to the pool without the overhead of
             mutex and semaphore acquisition/release/give.
            IMPLIES THAT THIS FUNCTION MUST ONLY RUN WHEN THERE IS ONLY
             ONE TASK RUNNING, ie in main(). */
        OS_semaphoreInitialise(&memory_pool->block_avail, number_of_blocks, number_of_blocks);

        /*  Add elements of correct size to the mempool by traversing the static_memory by
             'block_size' bytes and adding the traversed addresses to the pool. Higher
            Higher meory will be allocated first.
            The block head is cast to a 1B type so we can traverse
             the allocated memory addresses using N Bytes.*/
        uint8_t * block_head_add = (uint8_t *)static_memory;
        for (uint32_t i = 0; i < number_of_blocks; ++i) {
            memPool_add(memory_pool, block_head_add);
            /* Add 'block_size' bytes to the head */
            block_head_add += block_size;
        }
    } else {
        /*  If the memory pool is started without any memory blocks,
             start the semaphore on 0 rather than the added number of blocks */
        OS_semaphoreInitialise(&memory_pool->block_avail, number_of_blocks, 0);
    }
}

/**
 * [OS_memPoolAllocate Allocate a block of memory to active use from the pool.
 * 	If the pool is empty (and never deallocated to), this function will never
 * 	 return.
 * 	An item that has been in the pool should always be considered to be
 * 	 uninitialised, and will not be the same as when deallocated.]
 * @param  memory_pool [pointer to the OS_MemPool_t to deallocate to]
 * @return             [pointer to the allocated block of memory]
 */
void * OS_memPoolAllocate(OS_MemPool_t * memory_pool) {
    /* Take a semaphore to make sure there is a block available.
        When available, aquire the mutex for protection against concurrent access
        to the same memory. */
    OS_semaphoreTake(&memory_pool->block_avail);
    OS_mutexAcquire(&memory_pool->mutex_rw);

    /* Set the head pointing to the next available block */
    void ** pool_block = memory_pool->head;
    memory_pool->head = * pool_block;

    OS_mutexRelease(&memory_pool->mutex_rw);

    /* return the previous head to the user */
    return pool_block;
}

/**
 * [OS_memPoolDeallocate Deallocate a block of memory from use to the pool.
 * 	No more than number_of_blocks can be held in the pool, and the function will
 * 	 never return if the pool is full (and never allocated from).
 * 	When the item is deallocated, parts will instantly be overwritten. ]
 * @param memory_pool [pointer to the OS_MemPool_t to deallocate to]
 * @param item        [pointer to the block of memory to deallocate]
 */
void OS_memPoolDeallocate(OS_MemPool_t * memory_pool, void * const item) {
    /* No semaphore is used to protect against overfilling - this would
        have to be intentionally done on the users side due to how the initialisation
        works, and is hence left without protection instead of using 2 semaphores. */
    OS_mutexAcquire(&memory_pool->mutex_rw);

    memPool_add(memory_pool, item);

    /*  First give the semaphore, then release the mutex. It is done in this order
        to prioritise potential tasks waiting for the mutex over potential tasks
        waiting for the semaphore, apart from the statistically few cases where
        the scheduler performs a context switch in between these two lines, where
        the opposite effect will be seen. This prioritation is due to how notified
        waiting tasks are inserted into the next running task. */
    OS_semaphoreGive(&memory_pool->block_avail);
    OS_mutexRelease(&memory_pool->mutex_rw);

}

/**
 * [memPool_add  WARNING, this is not protected from concurrent access, corruption and overfilling.
     Must only be used through OS_memPoolDeallocate(), or via OS_memPoolInitialise() in
     the main prior to starting the OS via OS_start()
     Does exactly what OS_memPoolDeallocate does, but without mutex or semaphore
      protection for improved initialisation time.]
 * @param memory_pool [pointe to the memory pool to add to]
 * @param item        [the block to add to the pool]
 */
void memPool_add(OS_MemPool_t * memory_pool, void * const item) {
    /* Add the new item to the head of the list, and update the pool head to point to that.*/
    void ** mem_blocks = item;

    * mem_blocks = memory_pool->head;
    memory_pool->head = item;
}

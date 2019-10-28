#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_

#include <stdint.h>
#include "mutex.h"
#include "semaphore.h"

/*=============================================================================
 * This file is adding Memory Pool functionality to the OS, where the
 *   user can utilise these as a means of static malloc for a embedded system
 *   with predetermined structures /structure sizes.
===============================================================================
**       Example Use
*******************************************************************************
#include "mempool.h"
//TODO
=============================================================================*/


/*=============================================================================
**       Type Definitions
=============================================================================*/
/* A structure to hold a pointer to the last added memory block to the pool,
    and a mutex and semaphore for protection against exhaustion and
    corruption of the pool */
typedef struct {
	void * volatile head;
    OS_Mutex_t mutex_rw;
    OS_Semaphore_t block_avail;
} OS_MemPool_t;


/*=============================================================================
**       Function Prototypes
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
void OS_memPoolInitialise(OS_MemPool_t * memory_pool, void * const static_memory, const uint32_t number_of_blocks, const uint32_t block_size);

/**
 * [OS_memPoolDeallocate Deallocate a block of memory from use to the pool.
 * 	No more than number_of_blocks can be held in the pool, and the function will
 * 	 never return if the pool is full (and never allocated from).
 * 	When the item is deallocated, parts will instantly be overwritten. ]
 * @param memory_pool [pointer to the OS_MemPool_t to deallocate to]
 * @param item        [pointer to the block of memory to deallocate]
 */
void OS_memPoolDeallocate(OS_MemPool_t * memory_pool, void * const item);

/**
 * [OS_memPoolAllocate Allocate a block of memory to active use from the pool.
 * 	If the pool is empty (and never deallocated to), this function will never
 * 	 return.
 * 	An item that has been in the pool should always be considered to be
 * 	 uninitialised, and will not be the same as when deallocated.]
 * @param  memory_pool [pointer to the OS_MemPool_t to deallocate to]
 * @return             [pointer to the allocated block of memory]
 */
void * OS_memPoolAllocate(OS_MemPool_t * memory_pool);

#endif /* _MEMPOOL_H_ */

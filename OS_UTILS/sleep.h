#ifndef _SLEEP_H_
#define _SLEEP_H_

#include <stdint.h>
#include "task.h"

/*=============================================================================
 *  This file is adding sleep functionality to the OS.
 *   The maximum amount a single task can sleep in a single interval is
 *   (31^2 -1) ticks, or around 24.95 days.
===============================================================================
**       Example Use
*******************************************************************************
#include "mempool.h"
//TODO
=============================================================================*/


/*=============================================================================
**      Function Prototypes
=============================================================================*/
/**
 * [OS_sleep Put the current task to sleep for at least the provided value (ms).
 *  The task is guaranteed to be set to be set runnable again after the provided
 *   value of ticks, but the time until it actually runs after the sleep might be
 *   longer than this and depends on other tasks in the system.
 *  Must never be called outside a task.]
 * @param sleep_in_ms [time to wait in milliseconds  - must not be bigger than
    (31^2 -1) ticks (around 24.95 days) as behaviour will be unpredictable]
 */
void OS_sleep(const uint32_t sleep_in_ms);


/*=============================================================================
**      Internal Function Prototypes for OS Operation
=============================================================================*/
/**
 * [sleep_heapExtract Extracts the task which is to be awoken soonest]
 * @return  OS_TCB_t * [the pointer to the OS_TCB_t that was extracted]
 */
OS_TCB_t * sleep_heapExtract(void);

/**
 * [sleep_taskNeedsAwakening Returns whether the task at the top of the heap
 *   should be awoken or not]
 * @return  uint32_t [  1 if the top task should be awoken
 *                      0 if the top task should NOT be awoken]
 */
uint32_t sleep_taskNeedsAwakening(void);

#endif /* _SLEEP_H_ */

#include "sleep.h"
#include "task.h"
#include "roundRobin.h"
#include "mutex.h"
#include "os_internal.h"
#include "os_internal_def.h"
#include "debug.h"

/*  This file is adding sleep functionality to the OS using a minimum binary heap to
     maintain sleeping tasks in a semi-ordered state with the next task to be
     awoken at the top.
    As the potential for all tasks in an OS to be sleeping, it can
     accommodate for this.
    The sleep functionality is not affected by an overflowing SysTick counter,
     but as a result can only work with a maximum sleeping duration of
     (31^2 -1) ticks, around 24.95 days instead of (32^2 -1) or 49.9 days.

    This increases the static memory requirements of the OS by
        +   MAX_TASKS * 4 bytes     -   Minimum Binary Heap Array)
        +   4 bytes                 -   Heap length
        +   4 bytes                 -   Fail-Fast Counter
        +   12 bytes                -   Protective Mutex
        =   MAX_TASKS * 4 bytes + 20 bytes                  */

/*=============================================================================
**      Internal Macro Definitions
=============================================================================*/
/**
* [sleep_time1IsAfterTime2 Macro funtion calculates whether time1 is after time2
*    while taking into account potential overflows, based on modular arithmetic
*    on uint32_t.
*   This limits the sleep check to HALF_OF_UINT32_T_MAX, where any differences
*    equal to or over this (31^2 -1) will cause undefined behavour.]
* @param  time_1 [the time to check if is after 'time2']
* @param  time_2 [the second time to compare against]
* @param  ref_time  [the reference time to calculate time intervals from/to]
* @return uint32_t  [   1 if time_1 is after time_2 (including after overflow),
*                       0 if time_1 is equal to or before time_2]
*/
#define sleep_time1IsAfterTime2(time_1,time_2,ref_time) ( ( (uint32_t)( (uint32_t)(time_1)-(uint32_t)(ref_time) ) > \
                                                        (uint32_t)( (uint32_t)(time_2)-(uint32_t)(ref_time) )) )

/*=============================================================================
**      Static Function Prototypes
=============================================================================*/
static void sleep_heapUp(void);
static void sleep_heapDown(void);
static void sleep_heapInsert(OS_TCB_t * tcb);
static void sleep_heapSwapElements(uint32_t * elementIndexMain, uint32_t elementIndexSub);

/*=============================================================================
**      Static Variables
=============================================================================*/
/* A binary minimum heap to hold and partially sorted sleeping tasks,
    with the task to be soonest awoken always at the top, held in an array
    of pointers to TCBs with size MAX_TASKS to accommodate all tasks being
    able to sleep at once. */
static OS_TCB_t * volatile _heap_store[MAX_TASKS];
/* The length of the heap */
static uint32_t volatile _heap_length = 0;
/*  Fail-Fast counter check to make sure non-protected removal of sleeping tasks
     by the scheduler interrupts protected insertion of a task.
    This cannot be protected with mutexes as the scheduler cannot be waiting and
     hence should never be held by a mutex. */
static uint32_t volatile _sleep_fail_fast_counter = 0;
/* A mutex to protect simultaneous attempts at modification of the heap.
    To reduce any overhead, mutexes only surround the actual modifications,
    inside heapInsert and heapRemove. */
static OS_Mutex_t _sleep_mutex = {
    .tcb = 0,
    .counter = 0,
    .wait_queue_head = 0
};

/*=============================================================================
**      Functions
=============================================================================*/
/**
 * [OS_sleep Put the current task to sleep for at least the provided value (ms).
 *  The task is guaranteed to be set to be set runnable again after the provided
 *   value of ticks, but the time until it actually runs after the sleep might be
 *   longer than this and depends on other tasks in the system.
 *  Must never be called outside a task.
 *  Overflow of the SysTick is dealt with by comparing the difference of the
 *   two signed integers, and will work as long as the tasks are not set to sleep
 *   for more than half the full duration of the SysTick - maximum (2^32-1)/2 ticks.
 *  If this is adhered to, overflow of the SysTick should not cause
 *   erronous task awakenings. ]
 * @param sleep_in_ms [time to wait in milliseconds - must not be bigger than
    (31^2 -1) ticks (around 24.95 days) as behaviour will be unpredictable ]
 */
void OS_sleep(const uint32_t sleep_in_ms) {
    /* Get the current time as soon as possible to make the sleep as accurate
        as possible, not taking into account the extra scheduler overhead */
    uint32_t current_time = OS_elapsedTicks();

    /* Local pointer to the TCB to not call OS_currentTCB() multiple times */
    OS_TCB_t * tcb = OS_currentTCB();

    /* Store the time the tasks should be awoken in the tasks' data field.
        Overflow of this is handled in the internal functions when comparing
        awakening times. */
    tcb->data = current_time + sleep_in_ms;

    /*  Finally, insert the TCB and call _OS_removeTask(tcb) which will remove
         the TCB from the runnable tasks in the scheduler and trigger a task change.
        Checking the length of the queue to see if there is space is not necessary
         as we know the queue can hold all tasks in the system.
        These two statements must not be switched - the task must run until it
         has palced itself fully into the sleep heap. */
    sleep_heapInsert(tcb);
    _OS_removeTask(tcb);
}


/**
 * [sleep_taskNeedsAwakening Check whether the top element, if any,
 *  requires awakening.]
 * @return  [   1 if a top task exists and it requires awakening,
 *              0 otherwise]
 */
uint32_t sleep_taskNeedsAwakening(void) {
    /* If the sleep has no tasks, return immediately */
    if (!_heap_length) {
        return 0;
    }
    /* Make a common reference of the current time for the below function */
    uint32_t current_ticks = OS_elapsedTicks();

    /*  The top task requires awakening if the current ticks is after the
         awakening time by comparing the intervals between
         - the reference and current time
         - the reference and awakening time.
        This works even if one of the variables have overflown.
        The reason for adding the HALF_OF_UINT32_T_MAX ((31^2 -1)) is that the
         sleep_time1IsAfterTime2 function requires a common but unique reference,
         and this difference is thusly sized to maximise the possible sleep
         durations, up (31^2 -1) or ~24.9 days to from the current SysTick. */
    if (sleep_time1IsAfterTime2(current_ticks, _heap_store[0]->data, current_ticks + HALF_OF_UINT32_T_MAX) ) {
        return 1;
    }
    return 0;
}


/**
 * [sleep_heapInsert Internal function which inserts a task pointer into the
 *   sleep heap and maintains the partial ordering of the heap.
 *  This function is protecting against simultaneous modification to the heap
 *   by other tasks via a mutex.
 *  A potential race condition has been located, where the scheduler can feely
 *   context switch anywhere within this function, and if the top task is to be
 *   awoken it will perform sleep_heapExtract and modify the heap.
 *  This cannot be protected by mutexes as the scheduler should never not be able
 *   to perform its tasks. I have found no good way of dealing with this race condition.
 *  Protection against filling past the heap store allocated memory
 *   is not necessary as we know it cannot overflow due to the allocated space
 *   of MAX_TASKS.]
 * @param tcb [pointer to a OS_TCB_t to insert]
 */
static void sleep_heapInsert(OS_TCB_t * tcb) {
    OS_mutexAcquire(&_sleep_mutex);
    /*  The new element is always added to the end and sorted using heapUp.
        A potential race condition has been located where the scheduler can feely
         context switch anywhere within this function and sleep_heapUp, and if
         the top task is to be awoken it will perform sleep_heapExtract
         and modify the heap that this function already is modifying.
        This cannot be protected by mutexes as the scheduler should never not be
         able to perform its tasks. I have found no good way of dealing with
         this race condition.
        A worst case scenario would be that the scheduler preempts in between
         the two lines, where the tcb to be heapified was last, then moved up,
         and when returned here we would have a non-existent tcb to move up.
        As the heap and its length is volatile, this should still be fine,
         but it could also lead to heap corruption.
        To postulate, some sort of fail fast behaviour for the below two lines
         could work, but would still mean that the scheduler could potentially
         modify the currently modyfied heap.
        My best solution to this was to utilise a fail-fast counter checked prior
         to each actual swap in the heap internally in sleep_heapUp.
        This means that a scheduler modifying the heap will make it restart the
         current part of the do-while loop without making any changes that can
         now be wrong with the modified heap.*/
    _heap_store[_heap_length++] = tcb;
	sleep_heapUp();

    OS_mutexRelease(&_sleep_mutex);
}


/**
 * [sleep_heapExtract Extracts the root task pointer from the sleep heap.
    If the heap is empty, this will return arbitrary values, and should always be
     executed after a sleep_taskNeedsAwakening() check which also checks the
     heap is empty or not.
    As this will be run from within the scheduler, this cannot be protected
     by mutexes, and hence fail-fast behaviour has been added to
     sleep_heapInsert.]
 * @return  [a pointer to the task to be re-inserted in the scheduler]
 */
OS_TCB_t * sleep_heapExtract(void) {
	/*  The root element is extracted, and the end element is moved to root.
        The new root element is then sorted using heapDown */
	OS_TCB_t * tcb = _heap_store[0];
	_heap_store[0] = _heap_store[--_heap_length];
	sleep_heapDown();

    /* Increment the fail-fast counter so any potential tasks performing
        sleep_heapInsert knows the heap has been modified */
    _sleep_fail_fast_counter++;

	return tcb;
}


/**
 * [sleep_heapSwapElements Internal function to swap two indexed elements
 *  Main and Sub in the heap, referenced by their respecive heap indexes.
     NOTE: Only the Main element index will be updated.]
 * @param elementIndexMain [pointer to Heap Index of Main Element]
 * @param elementIndexSub  [heap Index of Sub Element]
 */
static void sleep_heapSwapElements(uint32_t * elementIndexMain, uint32_t elementIndexSub) {
    /* Swaps the two elements Main and Sub utilising a temporary tcb pointer for
        the intermediate stage. Finally updates the Main element index to its
        new respective position.*/
    OS_TCB_t * tmp_tcb;
    tmp_tcb = _heap_store[elementIndexSub];
    _heap_store[elementIndexSub] = _heap_store[*elementIndexMain];
    _heap_store[*elementIndexMain] = tmp_tcb;
    *elementIndexMain = elementIndexSub;
}


/**
 * [sleep_heapUp Internal function to sort an added element in its branch.
 *  Will swap elemement with its parent element until the awakening time of
 *   the parent is smaller the inserted element.
 *  This function has no arguments and returns nothing, but requires the new
 *   element to be partially sorted to have been added to the end/bottom of the heap.]
 */
static void sleep_heapUp(void) {
    /* Indexes for current TCB and Potential Parent TCBs.
        Additionally the current_time used for comparing time intervals,
        and the fast-fail-count to catch heap modifications made by the scheduler */
    uint32_t tcb_index, parent_tcb_index, current_time, fail_fast_count;

    /* Loop Control Variable */
    uint32_t element_is_bigger_than_parent = 1;

    /* Start with the last element of the heap, and proceed with heap up
        until parent awakening time is smaller than the current awakening time */
    tcb_index = _heap_length - 1;
    do {
        /*  Set the fast fail cont as early as possible within the loop - it has
            to be within the loop as if not it can never exit if the scheuler
            interrupts*/
        fail_fast_count = _sleep_fail_fast_counter;

        /* Check if root element and return if this is the case */
        if (tcb_index == 0){
            return;
        }

        /* Calculate the element parent depending on even or odd 1-indexed index.
           Efficiency could be improved by substituting (n_0 + 1) = n
            and swapping 0-indexed odd/even*/
        if ( (tcb_index + 1 ) % 2 == 0 ) {
            /* Even: 0-indexed parent = (n / 2) - 1 , where n = (tcb_index + 1) */
            parent_tcb_index = ( (tcb_index + 1) / 2) - 1;
        } else {
            /* Odd: 0-indexed parent = ((n - 1) / 2) - 1 , where n = (tcb_index + 1) */
            parent_tcb_index = ( tcb_index / 2) - 1;
        }

        /* Get the current time to use as reference when comparing which awakening
            time is sooner. */
        current_time = OS_elapsedTicks();

        /* Compare Current Element with Parent. If the current element should
            awake after its parent, exit the loop, else swap with parent and
            continue the loop */
        if (sleep_time1IsAfterTime2(_heap_store[tcb_index]->data, _heap_store[parent_tcb_index]->data, current_time + HALF_OF_UINT32_T_MAX) ) {
            element_is_bigger_than_parent = 0;
        } else {
            /* Do the swap between element and parent and update tcb_index only
                if the local fail-fast counter set at the start of the loop is
                the same as the current one incremented by the scheduler if it
                has edited the heap. */
            if (fail_fast_count == _sleep_fail_fast_counter) {
                sleep_heapSwapElements(&tcb_index, parent_tcb_index);
            }
        }
    } while (element_is_bigger_than_parent);
}


/**
 * [sleep_heapDown Internal function to sort the heap after extraction of the
 *   top element. This will swap the current elemement with the
     smallest of its children until the awakening time of the children
     are bigger than current element.
    This requires the last element to have been moved to the top prior to
     being called.]
 */
static void sleep_heapDown(void) {
	 /* Indexes for current TCB and Potential Children TCBs */
    uint32_t tcb_index, child_1_tcb_index, child_2_tcb_index, current_time;

     /* Control Variable and Loop Control Variable */
    uint32_t element_has_two_children, element_is_bigger_than_children = 1;

    /* Start with the root element, and proceed with heap down
        until child awakening time is bigger than or equal to
        the current tcb awakening time  */
    tcb_index = 0;
    do {
        /* Calculate the position of a potential first child using
            Child 1: child_1 = 2n , where n is (tcb_index+1).
           If current element has at least one child, continue */
        child_1_tcb_index = (2 * (tcb_index + 1)) - 1;
        if (_heap_length <= child_1_tcb_index) {
            return;
        }

        /*  If element has at least one child, check whether it has one or two.
            Calculate child 2 if necessary with determining if there is a second child:
            Child 2: child_2 = 2n+1 = child_1 + 1 , where n is (tcb_index+1). */
        if (_heap_length - 1 == child_1_tcb_index) {
            element_has_two_children = 0;
        } else {
            element_has_two_children = 1;
            child_2_tcb_index = child_1_tcb_index + 1;
        }

        /* Get the current time to use as reference when comparing which awakening
            time is sooner.*/
        current_time = OS_elapsedTicks();

        /* Return if element awake time is before both children or the only child,
            conditionally on having 2 or 1 children, respectively.*/
        if (element_has_two_children) {
            if (sleep_time1IsAfterTime2(_heap_store[child_1_tcb_index]->data, _heap_store[tcb_index]->data, current_time + HALF_OF_UINT32_T_MAX)
                    && sleep_time1IsAfterTime2(_heap_store[child_2_tcb_index]->data, _heap_store[tcb_index]->data, current_time + HALF_OF_UINT32_T_MAX) ) {
                element_is_bigger_than_children = 0;
                return;
            }
        } else {
            if (sleep_time1IsAfterTime2(_heap_store[child_1_tcb_index]->data, _heap_store[tcb_index]->data, current_time + HALF_OF_UINT32_T_MAX) ) {
                element_is_bigger_than_children = 0;
                return;
            }
        }

        /*  If code reached here, the element awaketime is bigger than at least one of its children's.
            If element has two children, check if child_1 > child_2 and swap with child_2,
             else swap with child_1 (in both 1 or two children cases). */
        if (element_has_two_children
                && sleep_time1IsAfterTime2(_heap_store[child_1_tcb_index]->data, _heap_store[child_2_tcb_index]->data, current_time + HALF_OF_UINT32_T_MAX) ) {
            sleep_heapSwapElements(&tcb_index, child_2_tcb_index);
        } else {
            sleep_heapSwapElements(&tcb_index, child_1_tcb_index);
        }
    } while (element_is_bigger_than_children);
}

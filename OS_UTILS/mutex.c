#include "mutex.h"
#include "os_internal.h"
#include "wait.h"
#include "stm32f4xx.h"
#include "os_internal_def.h"

/**
 *  This file contains the Mutual Exlusion (MutEx) specific section
 *   of the wait functionality in DocetOS, further provided in wait.c .
 *  It implements highly timing critical sections and should only be modified
 *  with deep appreciation of the OS and potential race conditions.
 */

/*=============================================================================
**      Functions
=============================================================================*/
/**
 * [OS_mutexInitialise  Initialises the mutex.]
 * @param mutex [pointer to a OS_Mutex_t]
 */
void OS_mutexInitialise (OS_Mutex_t * mutex) {
    mutex->tcb = 0;
    mutex->counter = 0;
    mutex->wait_queue_head = 0;
}

/**
 * [OS_mutexAcquire Aquires the mutex.
 *  This function is highly timing critical, and edits should be made with
 *   caution to not allow race conditions.
 *  The current task will wait forever if the resource is never made available.
 *  Mutex is acquired through atomic LDREX/STREX operations to protect from
 *   concurrent access - if successful it will clear the processor exclusive
 *   access flag if set prior to entering this function.
 *  Entering wait state is protected from concurrent notification using a
 *   fail-fast check code incremented in _OS_notify - if not equal in _OS_wait
 *   as in this function, return and try again.
 *  A memory barrier (DMB) is used as recommended by ARM after acquiring the mutex,
 *   although not strictly necessary on the M4 (see
 *   http://infocenter.arm.com/help/topic/com.arm.doc.dai0321a/BIHEJCHB.html)]
 * @param mutex [pointer to a OS_Mutex_t]
 */
void OS_mutexAcquire(OS_Mutex_t * mutex) {
    uint32_t fail_fast_check;
    OS_TCB_t * mutex_tcb;
    /*  Try to retrieve mutex until either:
            a) mutex is available - take the mutex.
            b) mutex is not available -> try to wait for resource
        CMSIS compiler-specific primitives for atomic LDREX and STREX are used:
            uint32_t __LDREXW (uint32_t *addr);
            uint32_t __STREXW (uint32_t value, uint32_t *addr);
        These primitives atomically load and store data to and from *addr,
         successfull if CLREX has not been called to reset the processor
         exclusive access (PEA) flag in between the two calls.
        The flag is reset every context switch and after use of STREX. */
    while (RESOURCE_NOT_AQUIRED) {
        /*  Set the fast-fail check counter as early within the loop as possible,
             to catch any tasks that release the mutex in the middle of this execution */
        fail_fast_check = OS_currentFastFailCounter();

        /*  Atomically load the the mutex owner (LDREX) - will set the PEA flag */
        mutex_tcb = (OS_TCB_t *)__LDREXW((uint32_t *)&mutex->tcb);

        /*  If the mutex is not already acquired, try to atomically acquire it (STREX):
            If successfully acquired, set a memory barrier and exit the loop.
            STREX requires the PEA flag to be set to be successful, and will clear it.
            If mutex is already acquired, check whether the current task owns it,
             and if so increment the recursive counter.
            Otherwise, try to put the current task to wait (will return
             immediately if fail_fast_check does not equal the
             OS_currentFastFailCounter() within wait). */
        if (mutex_tcb == 0) {
            if (__STREXW((uint32_t)OS_currentTCB(), (uint32_t *)&mutex->tcb) == STREXW_SUCCESSFUL) {
                /*  Mutex was successfully acquired.
                    A memory barrier is recommended by ARM, but not necessary on M4.
                    Do not start any other memory access until memory barrier is completed. */
                __DMB();
                break;
            }
        } else {
            if (mutex_tcb == OS_currentTCB() ) {
                /* Mutex was already acquired. No memory barrier required as
                    it's already in place. */
                break;
            } else {
                /*  Mutex was unavailable - call fail-fast _OS_wait, and try to
                     re-acquire mutex once returned (either due to fail-fast
                     behaviour or available mutex).
                    If mutex is never made available this function will never exit.*/
                _OS_wait(mutex, (void *)&mutex->wait_queue_head, fail_fast_check);
            }
        }
    }
    /* If the code gets here, the mutex is either acquired or re-acquired.
        Will return for MutExed section after incrementing recursive counter. */
    mutex->counter++;
}

/**
 * [OS_mutexRelease Release the mutex if the current TCB is the owner
 *   (it always should be).
 *  This function is somewhat timing critical, and edits should be made with
 *   caution to not allow race conditions.
 *  A memory barrier (DMB) is used as recommended by ARM before releasing the mutex,
 *   although not strictly necessary on the M4:
 *    (see http://infocenter.arm.com/help/topic/com.arm.doc.dai0321a/BIHEJCHB.html).
 *  If the recursive count has reached 0, meaning this was the last of a balanced
 *   set of acquire and release calls, notify waiting tasks for the
 *   now available resource. ]
 * @param mutex [pointer to a OS_Mutex_t]
 */
void OS_mutexRelease(OS_Mutex_t * mutex) {
    /*  Release the mutex, making sure only the task that owns the mutex can do so.
        There should never be a case where this occurs unless malignantly placed
         by user.
        If the task no longer holds the mutex, clear the mutex and notify waiting tasks. */
    if (OS_currentTCB() == mutex->tcb) {
        /*  Recommended by ARM, but not necessary on M4.
            Ensure memory operations completed before releasing lock */
        __DMB();
        mutex->counter--;
        if (mutex->counter == 0) {
            mutex->tcb = 0;
            /*  Potential race condition here if another task that hasn't been
                 waiting concurrently tries to acquire the mutex here,
                 at which point it should succeed.
                We then have a state where the mutex is acquired, but notify will
                 be called for the same mutex next time this task runs again.
                This will, if the mutex has any waiting tasks, make the first
                 waiting task runnable and run on the next context switch, only
                 to be set to wait again as the mutex was already acquired by
                 the other task that hadn't waited in the first place.*/
            _OS_notify( (void *)&mutex->wait_queue_head );
        }
    }
}

#include "semaphore.h"
#include "os_internal.h"
#include "wait.h"
#include "stm32f4xx.h"
#include "os_internal_def.h"
#include "debug.h"

/**
 *  This file contains the synchonisation tool (semaphores) specific section
 *   of the wait functionality in DocetOS, further provided in wait.c .
 *  It implements highly timing critical sections and should only be modified
 *  with deep appreciation of the OS and potential race conditions.
 */

/*=============================================================================
**      Functions
=============================================================================*/

/**
 * [OS_semaphoreInitialise  Initialises a counting semaphore. It will block tasks
 *   when either empty and full, apart from when size = 0 where it will
 *   only block task when empty.]
 * @param semaphore   [pointer to the OS_Semaphore_t to initialise]
 * @param size        [The size of the semaphore. This can either be:
 *   size > 1 - will block tasks trying to take when empty and give when full
 *   size = 0 - will block tasks trying to take when empty, but has no upper bound]
 * @param init_tokens [the number of tokens to initialise the semaphore with.
 *  This is checked for being between 0 and 'size', and set to 'size' if above]
 */
void OS_semaphoreInitialise (OS_Semaphore_t * semaphore, const uint32_t size, const uint32_t init_tokens) {
    semaphore->max_tokens = size;
    /* Make sure the number of initial tokens are not more than the semaphore capacity,
        notify the user in debug to make sure the error is not intentional
        as the statement will reset it to 'size'. */
    uint32_t _init_tokens = init_tokens;
    if (init_tokens > size) {
        ASSERT_DEBUG(0);
        _init_tokens = size;
    }
    semaphore->tokens = _init_tokens;
    semaphore->wait_queue_head = 0;
}


/**
 * [OS_semaphoreInitialiseBinary Initialises a binary semaphore.
 *  It will block waiting tasks both when empty(0) and full(1), and is a special
 *   case of OS_semaphoreInitialise with size=1.]
 * @param semaphore [pointer to the OS_Semaphore_t to initialise]
 * @param init_full [the number of tokens to initialise with, (1 or 0)]
 */
void OS_semaphoreInitialiseBinary(OS_Semaphore_t * semaphore, const uint32_t init_full) {
    semaphore->max_tokens = 1;
    /* Initialise token to 1 or 0 depending on if(init_full).
       If larger than 1, set to 1, notify user in debug to make sure the error
        is not intentional as the statement will reset it to '1'.*/
    uint32_t _init_full = init_full;
    if (init_full > semaphore->max_tokens) {
        ASSERT_DEBUG(0);
        _init_full = semaphore->max_tokens;
    }
    semaphore->tokens = _init_full;
    semaphore->wait_queue_head = 0;
}


/**
 * [OS_semaphoreInitialiseCounting Initialises a counting semaphore with size
 *   and starting tokens equal to 0, a special case of OS_semaphoreInitialise.
 *  This semaphore has no ceiling, and can be given (2^32)-1 tokens before.
 *   overflowing. The overflow is up to the user to prevent if relying on
 *   an equal number of gives and takes, but will not effect OS_semaphoreGive
 *   from continueing after the overflow.]
 * @param semaphore [pointer to the OS_Semaphore_t to initialise]
 */
void OS_semaphoreInitialiseCounting(OS_Semaphore_t * semaphore) {
    semaphore->max_tokens = 0;
    semaphore->tokens = 0;
    semaphore->wait_queue_head = 0;
}

/**
 * [OS_semaphoreTake Takes a semaphore.
 *  This function is highly timing critical, and edits should be made with
 *   caution to not allow race conditions.
 *  The current task will wait forever if a token is never available.
 *  Once this function returns, a token has been successfully taken.
 *  Token is taken through atomic LDREX/STREX operations to protect from
 *   concurrent access - if successful it will clear the processor exclusive
 *   access flag if set prior to entering this function.
 *  Entering wait state is protected from concurrent notification using a
 *   fail-fast check code incremented in _OS_notify - if not equal in _OS_wait
 *   as in this function, return and try again.]
 * @param semaphore [pointer to the OS_Semaphore_t to take a token from]
 */
void OS_semaphoreTake(OS_Semaphore_t * semaphore) {
    uint32_t token_counter, fail_fast_check;

    /*  Try to take a semaphore token until either:
            a) token is available - take the token.
            b) token is not available -> try to wait for a token to be given
        CMSIS compiler-specific primitives for atomic LDREX and STREX are used:
            uint32_t __LDREXW (uint32_t *addr);
            uint32_t __STREXW (uint32_t value, uint32_t *addr);
        These primitives atomically load and store data to and from *addr,
         successfull if CLREX has not been called to reset the processor
         exclusive access (PEA) flag in between the two calls.
        The flag is reset every context switch and after use of STREX. */
    while (RESOURCE_NOT_AQUIRED) {
        /*  Set the fast-fail check counter as early within the loop as possible,
             to catch any tasks that give a token in the middle of this execution */
        fail_fast_check = OS_currentFastFailCounter();

        /*  Atomically load the the semaphore token (LDREX) - will set the PEA flag */
        token_counter = __LDREXW(&semaphore->tokens);

        /*  If the semaphore has available tokens, try to atomically take one (STREX):
            If successfully taken, set a memory barrier and notify other tasks
             that a token was taken.
            STREX requires the PEA flag to be set to be successful, and will clear it.
            If no tokens are available, try to put the current task to wait
             (will return immediately if fail_fast_check does not equal the
             OS_currentFastFailCounter() within wait). */
        if (token_counter > 0) {
            if (__STREXW(--token_counter, &semaphore->tokens) == STREXW_SUCCESSFUL) {
                /* Token was successfully taken. Notify tasks waiting to give a token.*/
                _OS_notify( (void *)&semaphore->wait_queue_head );
                return;
            }
        } else {
            /*  There were no token navailable - call fail-fast _OS_wait, and try
                 to re-acquire a token once returned (either due to fail-fast
                 behaviour or available token).
                If token is never made available this function will never exit.*/
			_OS_wait(semaphore, (void *)&semaphore->wait_queue_head, fail_fast_check);
        }
    }
}

/**
 * [OS_semaphoreTake Gives a semaphore.
 *  This function is highly timing critical, and edits should be made with
 *   caution to not allow race conditions.
 *  The current task will wait forever if the semaphore is full and no token are
 *   removed.
 *  Once this function returns, a token has been successfully given.
 *  Token is given through atomic LDREX/STREX operations to protect from
 *   concurrent access - if successful it will clear the processor exclusive
 *   access flag if set prior to entering this function.
 *  Entering wait state is protected from concurrent notification using a
 *   fail-fast check code incremented in _OS_notify - if not equal in _OS_wait
 *   as in this function, return and try again.]
 * @param semaphore [pointer to the OS_Semaphore_t to take a token from]
 */
void OS_semaphoreGive(OS_Semaphore_t * semaphore) {
    uint32_t token_counter, fail_fast_check;

    /* Give back a semaphore token, and notify any waiting tasks regardless of if there are any,
        that there is a token available. */

    /*  Try to give a semaphore token until either:
            a) the semaphore is not full - give the token.
            b) the semaphore is full -> try to wait until a token is taken
        CMSIS compiler-specific primitives for atomic LDREX and STREX are used:
            uint32_t __LDREXW (uint32_t *addr);
            uint32_t __STREXW (uint32_t value, uint32_t *addr);
        These primitives atomically load and store data to and from *addr,
         successfull if CLREX has not been called to reset the processor
         exclusive access (PEA) flag in between the two calls.
        The flag is reset every context switch and after use of STREX. */
    while (RESOURCE_NOT_RETURNED) {
        /*  Set the fast-fail check counter as early within the loop as possible,
             to catch any tasks that give a token in the middle of this execution */
        fail_fast_check = OS_currentFastFailCounter();

        /*  Atomically load the the semaphore token (LDREX) - will set the PEA flag */
        token_counter = __LDREXW(&semaphore->tokens);

        /*  If the semaphore is not full or if this is an un-capped
             semaphore (max_tokens == 0), try to atomically give one token (STREX):
            If successfully given, set a memory barrier and notify other tasks
             that a token was taken.
            STREX requires the PEA flag to be set to be successful, and will clear it.
            If the semaphore is full, try to put the current task to wait
             (will return immediately if fail_fast_check does not equal the
             OS_currentFastFailCounter() within wait). */
        if (token_counter < semaphore->max_tokens || semaphore->max_tokens == 0 ) {
            /* Token was successfully taken. Notify tasks waiting to give a token. */
            if (__STREXW(++token_counter, &semaphore->tokens) == STREXW_SUCCESSFUL) {
                _OS_notify( (void *)&semaphore->wait_queue_head );
                return;
            }
        } else {
            /*  The semaphore was full and we could not give a token - call
                 fail-fast _OS_wait, and try to give a token once
                 returned (either due to fail-fast behaviour or that the
                 semaphore is no longer full).
                If semaphore is never emptied, this function will never exit.*/
            _OS_wait(semaphore, (void *)&semaphore->wait_queue_head, fail_fast_check);
        }
    }
}

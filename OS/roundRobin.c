#include "roundRobin.h"
#include "stm32f4xx.h"
#include "mutex.h"
#include "semaphore.h"
#include "wait.h"
#include "sleep.h"
#include "debug.h"

/* This is an implementation of a fixed priority round-robin scheduler similar
     to that in FreeRTOS.
    Priorities go from PRIORITY_MAX down to 1, with only the system idle task at
    a lower priority. */

/*=============================================================================
**      Static Function Prototypes
=============================================================================*/
static OS_TCB_t const * roundRobin_scheduler(void);
/* Adding and Removing tasks from the OS completely*/
static void roundRobin_addTask(OS_TCB_t * const tcb);
static void roundRobin_exitTask(OS_TCB_t * const tcb);
/* Insert and Removes tasks into and from the scheduler for sleep and wait mechanisms */
static void roundRobin_insertTask(OS_TCB_t * const tcb);
static void roundRobin_removeTask(OS_TCB_t * const tcb);
/* Removes tasks from the scheduler if a resource is unavialable when requested,
    or notifies the first task waiting for a resource that has been made available.*/
static void roundRobin_wait(void * const reason, void * const unavailable_resource_wait_queue_head, uint32_t fail_fast_counter);
static void roundRobin_notify(void * const available_resource_wait_queue_head);


/*=============================================================================
**      Static Variables
=============================================================================*/
#ifndef DEBUG_HARD
/* Hold pointers to the most recently active task in each priority, or 0 if no tasks in that priority.
    Will unfortunately hold 4 bytes more than necessary due to that priority 0 is not used, but
     this cleans up code so much that it is worth it. */
static OS_TCB_t * _tasks_pri[PRIORITY_LEVELS] = {0};
#else
/* If in DEBUG_HARD, define as NON-static to be able to add to watches outside
    this translation unit, as well as A debug array for all tasks to be able
    to explore them without 'traversing' the linked lists. */
OS_TCB_t * _tasks_pri[PRIORITY_LEVELS] = {0};
OS_TCB_t * _debug_tasks[MAX_TASKS] = {0};
#endif

/* Variable to hold number of currently added tasks (incl. sleeping and waiting tasks, excl. idle task),
    to make sure that tasks aren't added over the scheduler capacity set by MAX_TASKS. The limitation
    is implemented to make sure the sleep heap is sufficiently sized for all tasks to be asleep at the same time.  */
static uint8_t _tasks_added = 0;

/*=============================================================================
**      Scheduler Declaration and Instantiation
=============================================================================*/
/* Scheduler block for the fixed priority round-robin scheduler */
OS_Scheduler_t const round_robin_scheduler = {
	.preemptive = 1,
	.scheduler_callback = roundRobin_scheduler,
	.taskAdd_callback = roundRobin_addTask,
    .taskExit_callback = roundRobin_exitTask,
    .taskRemove_callback = roundRobin_removeTask,
	.wait_callback = roundRobin_wait,
    .notify_callback = roundRobin_notify
};

/*=============================================================================
**      Functions
=============================================================================*/

/**
 * [roundRobin_scheduler The scheduler call back. Returns the first task of
 *  the highest priority, starting at the top of the priority buckets.]
 * @return  [pointer to the next task to be run]
 */
static OS_TCB_t const * roundRobin_scheduler(void) {
    /* For-loop to return the next task, higher priorities first.
        If there are tasks in the highest priority, return the next TCB held in the TCB
        If there are no tasks in the priority, search for tasks in the next priority until one is found.
        If no tasks are runnable, return the Idle task. */

    /* Check whether any tasks should be awoken.
        This could be improved by a hardware timer
        until the next awakening, triggering a ISR to insert it again, which
        means no time waisted on polling the top sleep */
    while( sleep_taskNeedsAwakening() ) {
        roundRobin_insertTask(sleep_heapExtract());
    }

    /*  Return the first task in the highest priority, or the idle task if
        none of the buckets have tasks / doubly-linked lists in them */
    for (uint_fast8_t priority = PRIORITY_MAX; priority > 0; priority--) {
        if(_tasks_pri[priority] == 0) {
            continue;
        } else {
            _tasks_pri[priority] = _tasks_pri[priority]->next;
            return _tasks_pri[priority];
        }
    }

    /* No tasks active; return the idle task */
	return OS_idleTCB_p;
}

/**
 * [roundRobin_addTask Initially adds a task to the runnable tasks]
 * @param tcb [pointer to the tcb to add]
 */
static void roundRobin_addTask(OS_TCB_t * const tcb) {
    /* Make sure not too many tasks are added. If user tries to add
        past MAX_TASKS tasks, nothing will be added and no warning
        will occur in non-debug mode. This will stop further
        execution in debug modes. */
    if(_tasks_added >= MAX_TASKS) {
        ASSERT_DEBUG(0);
        return;
    }

    /* Add the task to the doubly linked list of the right priority */
    roundRobin_insertTask(tcb);
    _tasks_added++;

    #ifdef DEBUG_HARD
    /* Add the task to debug array if in debug mode */
    _debug_tasks[_tasks_added-1] = tcb;
    #endif
}

/**
 * [roundRobin_exitTask Removes a task completely when it has finished running.]
 * @param tcb [pointer to task to remove]
 */
static void roundRobin_exitTask(OS_TCB_t * const tcb) {
    /* Extract the task from the scheduler and update the number of total tasks in the OS.
        This will only run when the task is finished, called by the _svc_OS_taskExit and related handlers*/
    roundRobin_removeTask(tcb);
    _tasks_added--;
}

/**
 * [roundRobin_insertTask Inserts a task from wait or sleep back into the scheduler.]
 * @param tcb [pointer to the TCB to insert]
 */
static void roundRobin_insertTask(OS_TCB_t * const tcb) {
    /*  Add the TCB to the circularly doubly-linked lists of correct priority,
         with different behaviour depending on if the linked list within the
         priority is empty or not.
        Only using tcb->next for checking if the list has >1 tasks in removeTask,
         so we don't need to set tcb->prev in the empty case (only useful
         with more than 1 task, at which point it will be set by the addition
         of the new task). */
    if(_tasks_pri[tcb->priority] == 0) {
        /* If no tasks in the given priority, link the task->next to itself */
        _tasks_pri[tcb->priority] = tcb;
        tcb->next = tcb;
    } else {
        /* There are one or more tasks in the same priority - insert between current
            and next task. */
        tcb->prev = _tasks_pri[tcb->priority];
        tcb->next = _tasks_pri[tcb->priority]->next;
        (tcb->prev)->next = tcb;
        (tcb->next)->prev = tcb;
    }
}

/**
 * [roundRobin_removeTask Removes a task from the scheduler, ie when it goes to
 *  wait or sleep]
 * @param tcb [pointer to the TCB to remove]
 */
static void roundRobin_removeTask(OS_TCB_t * const tcb) {
    /* Remove the task from the doubly linked list. */
    if (tcb->next == tcb) {
        _tasks_pri[tcb->priority] = 0;
    } else {
        (tcb->prev)->next = tcb->next;
        (tcb->next)->prev = tcb->prev;
        /* Update the pointer to the previous task, so the next scheduler run will run the current tcb->next  */
        _tasks_pri[tcb->priority] = tcb->prev;
    }

}

/**
 * [roundRobin_wait Sets a task to wait for a resource as long as the fast-fail_fast_count
 *  has not been incremented. Then schedules a task switch. ]
 * @param unavailable_resource                 [the semaphore or mutex that is unavialable]
 * @param unavailable_resource_wait_queue_head [the task at the head of the queue]
 * @param fail_fast_counter                    [the fail fast check code]
 */
static void roundRobin_wait(void * const unavailable_resource, void * const unavailable_resource_wait_queue_head, uint32_t fail_fast_counter) {
    /* Only initiate wait if no task notifications have occcurred
        between current task called _OS_wait() and here. */
    if (fail_fast_counter == OS_currentFastFailCounter()) {
		/* Insert the now waiting task into the wait queue,
            remove it from the runnable scheduler tasks,
			and finally invoke the scheduler.
            This NEEDS to happen before queueInsert as we are modifying the ->next field. */
        roundRobin_removeTask(OS_currentTCB());
        wait_queueInsert( (OS_TCB_t **)unavailable_resource_wait_queue_head, OS_currentTCB());
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }
}


/**
 * [roundRobin_notify Notify a task of available resource.]
 * @param available_resource_wait_queue_head [the head of the wait queue to be notified.]
 */
static void roundRobin_notify(void * const available_resource_wait_queue_head) {
    /* Make the highest priority tasks, that requested this resource first
        when uavailable, runnable, (if any waiting tasks). */
    OS_TCB_t * waiting_task = wait_queueExtract( (OS_TCB_t **)available_resource_wait_queue_head );
    if (waiting_task != 0) {
        roundRobin_insertTask(waiting_task);
    }
}

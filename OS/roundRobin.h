#ifndef _ROUNDROBIN_H_
#define _ROUNDROBIN_H_

#include "os.h"

/*=============================================================================
 *  This is an implementation of a fixed priority round-robin scheduler similar
 *    to that in FreeRTOS.
 *   Priorities go from PRIORITY_MAX down to 1, with only the system idle task at
 *    a lower priority.
=============================================================================*/

/*=============================================================================
**       Global Scheduler Declaration
=============================================================================*/
/* The global scheduler that should be initialised before adding tasks and starting
    the OS. */
extern OS_Scheduler_t const round_robin_scheduler;

/*=============================================================================
**       Definitions
=============================================================================*/
/*****************************************************************************
**      USER MODIFIABLE CONFIGURATION - START
**      ONLY MODIFY DEFINITIONS DONE IN BETWEN START AND END TAGS
******************************************************************************/
/*  Sets the maximum number of tasks that is allowed to run on this scheduler.
    This is user modifiable: An increase in tasks increases the static storage
     allocated to the OS for sleep functionality, and should be used in 'small'
     systems only due to the schedulers reduced overhead by not implementing
     more intricate features required in larger systems such as aging and resource
     starvation. */
#define MAX_TASKS 15

/*  Number of different priority levels - a higher priority is prioritised by
`    the scheduler over lower priorities.
    The user should also consider the overhead added by increasing this number,
     and should aim for a lower number of different priorities if possible.
    The priorities are 1-indexed from PRIORITY_MAX (PRIORITY_LEVELS - 1)
     down to PRIORITY_MIN (1). */
#define PRIORITY_LEVELS 5
/*****************************************************************************
**      USER MODIFIABLE CONFIGURATION - END
**      DO NOT MODIFY ANYTHING BELOW THIS LINE
******************************************************************************/

/* The max priority that can be used with the number of PRIORITY_LEVELS set.
    The lowest priority that can be used is 0. */
#define PRIORITY_MAX (PRIORITY_LEVELS - 1)


/*=============================================================================
**       Error checking of Modifiable Definitions Above, DO NOT EDIT
=============================================================================*/
#if MAX_TASKS <= 0
# error "MAX_TASKS must be bigger than 0. Please increase MAX_TASKS.."
#endif

#if PRIORITY_LEVELS < 1
# error "PRIORITY_LEVELS must be at least 1. Please increase PRIORITY_LEVELS.."
#endif

#endif /* _ROUNDROBIN_H_ */

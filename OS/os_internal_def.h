#ifndef _OS_INTERNAL_DEF_H_
#define _OS_INTERNAL_DEF_H_

/*=============================================================================
 *      Definitions for Internal Use within the OS
=============================================================================*/
/*****************************************************************************
**  Used by mutex.c and semaphore.c
******************************************************************************/
#define RESOURCE_NOT_AQUIRED 1
#define RESOURCE_NOT_RETURNED 1
#define STREXW_SUCCESSFUL 0

/*****************************************************************************
**  Used by sleep.c
******************************************************************************/
/* Half the size of uint32_MAX */
#define HALF_OF_UINT32_T_MAX 0x7FFFFFFF

#endif /* _OS_INTERNAL_DEF_H_ */

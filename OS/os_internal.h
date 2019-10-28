#ifndef _OS_INTERNAL_H_
#define _OS_INTERNAL_H_

#include "os.h"
#include "task.h"

#define ASSERT(x) do{if(!(x))__breakpoint(0);}while(0)

#include "stm32f4xx.h"

typedef struct {
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	const volatile uint32_t r12;
	const volatile uint32_t lr;
	const volatile uint32_t pc;
	const volatile uint32_t psr;
} _OS_SVC_StackFrame_t;

/* Globals */
extern OS_TCB_t * volatile _currentTCB;

/* svc */
void __svc(OS_SVC_EXIT) _OS_task_exit(void);

/* C */
void _OS_task_end(void);

/* asm */
void _task_switch(void);
void _task_init_switch(OS_TCB_t const * const idleTask);

#endif /* _OS_INTERNAL_H_ */


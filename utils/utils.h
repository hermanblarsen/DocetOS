#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "../OS/os.h"

/********************/
/*  Prototypes ASM  */
/********************/
uint32_t getPSR(void);
uint32_t getCONTROL(void);

/********************/
/*  Prototypes      */
/********************/
void reportState(void);

/********************/
/*  Prototypes SVC  */
/********************/
uint32_t __svc(SVC_TEST_DELEGATE) test_delegate(uint32_t); //with int in and out - delegate will not have this, but will utilise stack
//void __svc(SVC_TEST_DELEGATE) test_delegate(void); //without args


#endif /* UTILS_H */

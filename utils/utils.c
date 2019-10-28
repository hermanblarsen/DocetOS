#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include "os_internal.h"



void reportState(void) {
    uint32_t psr, controlReg;
    uint_fast8_t ISR_num;

    psr = getPSR();
    controlReg = getCONTROL();
    
    printf("=====FUNC ReportState\n\r\tPSR: 0x%x , ControlReg: 0x%x, ", psr, controlReg);
    
    //Check bits 8:0 (9 bits) using bitwise OR,  handler mode if any bit is 1, else thread mode
    ISR_num = psr & 0x1FF; 
    
    //Check bit 0 (nPRIV) to find out whether thread mode code is privileged 
    // (0 means yes, 1 means no (but handler is always priveliged)).
    if (ISR_num) {
        printf("Mode: HANDLER, ISR#: 0x%x, Privelige: YES, ", ISR_num);
    } else {
        printf("Mode: THREAD, Privelige: %s, ", (controlReg & 1UL<<0) ? "NO" : "YES");
    }
    
    //Check bit 1 (SPSEL) to find out whether the MSP (0) or the PSP (1) is the active stack pointer
    printf("Active SP: %s\n\r", (controlReg & 1UL<<1) ? "PSP" : "MSP");
}

void _svc_test_delegate(_OS_SVC_StackFrame_t * const stack) {
    printf("=====FUNC _svc_test_delegate, calling reportState\n\r"); 
    reportState();
    printf("\tArgument Passed: r0: 0x%x\n\r", stack -> r0); 
    stack -> r0 = stack -> r0 << 1; //shift by 1 (times by 2)
}



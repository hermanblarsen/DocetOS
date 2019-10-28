#include "os.h"
#include <stdio.h>
#include "utils/serial.h"
#include "simpleRoundRobin.h"
#include "stm32f4xx.h"
#include "utils/utils.h"

void task1(void const *const args) {
    printf("0x%x", test_delegate(3));
    for (int i = 0; i < 100; ++i) {
		printf("A");
	}
    printf("enda");
}

void task2(void const *const args) {
    for (int i = 0; i < 200; ++i) {
		printf("B");
	}
    printf("endb");
}

void task3(void const *const args) {
    for (int i = 0; i < 300; ++i) {
		printf("C");
	}
    printf("endc");
}

/* MAIN FUNCTION */

int main(void) {
	/* Initialise the serial port so printf() works */
	serial_init();

	printf("\r\nDocetOS Demo\r\n");

	/* Reserve memory for two stacks and two TCBs.
	   Remember that stacks must be 8-byte aligned. */
	__align(8)
	static uint32_t stack1[60], stack2[64], stack3[64];
	static OS_TCB_t TCB1, TCB2, TCB3;

	/* Initialise the TCBs using the two functions above */
	//Args: Address of the TCB, the top+1 of the stack (since full descending), the task and NULL argument
	OS_initialiseTCB(&TCB1, stack1+60, task1, NULL);
	OS_initialiseTCB(&TCB2, stack2+64, task2, NULL);
	OS_initialiseTCB(&TCB3, stack3+64, task3, NULL);
	
	/* Initialise and start the OS */
    printf("Prior to OS_Init and/or start, calling reportState\n\r"); 
    reportState();
	OS_init(&simpleRoundRobinScheduler);
	OS_addTask(&TCB1);
	OS_addTask(&TCB2);
    OS_addTask(&TCB3);
	OS_start();
}

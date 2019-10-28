/******************************************************************************/
/* RETARGET.C: 'Retarget' layer for target-dependent low level functions      */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005-2006 Keil Software. All rights reserved.                */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/
/* Modified by AJP for University of York laboratory sessions, 2012-2013 	    */
/******************************************************************************/

#include <stdio.h>
#include <stm32f4xx.h>

#pragma import(__use_no_semihosting_swi)

// Redirect output via USART2 (AJP 2013)
// Calls to this function block on USART2 TX buffer availability
int sendchar(int c) {
	while (!(USART2->SR & USART_SR_TXE));
	return (USART2->DR = c);
}

struct __FILE { int handle; };
FILE __stdout;

int fputc(int ch, FILE *f) {
  return (sendchar(ch));
}


int ferror(FILE *f) {
  return EOF;
}


void _ttywrch(int ch) {
  sendchar(ch);
}


void _sys_exit(int return_code) {
	// Endless loop
  while(1);
}

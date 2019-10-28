#include "serial.h"
#include "stm32f4xx.h"

static void _configUSART2(uint32_t BAUD)
{
	static uint16_t AHBdiv[] = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 8, 16, 64, 128, 256, 512 };
	static uint8_t APBdiv[] = { 1, 1, 1, 1, 2, 4, 8, 16 };
  uint32_t apb1clock = 0x00;
  uint32_t integerdivider = 0x00;

  apb1clock = (HSE_VALUE/PLL_M*PLL_N/PLL_P) / AHBdiv[(RCC->CFGR & RCC_CFGR_HPRE) >> 4] / APBdiv[(RCC->CFGR & RCC_CFGR_PPRE1) >> 10];
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;	/* Enable USART2 Clock */

	GPIOA->MODER &= ~GPIO_MODER_MODER2;
  GPIOA->MODER |=  GPIO_MODER_MODER2_1;		/* Setup TX pin for Alternate Function */

  GPIOA->AFR[0] |= (7 << (4*2));		/* Setup TX as the Alternate Function */

  USART2->CR1 |= USART_CR1_UE;	/* Enable USART */

  integerdivider = (1000 * (uint64_t)apb1clock / BAUD); /* 1000 x baud rate divider, OVER8=0 */
  USART2->BRR = (uint16_t)((integerdivider + 500) / 1000);

  USART2->CR1 |= USART_CR1_TE;	/* Enable Tx */
}

void serial_init(void) {
	_configUSART2(38400);
}

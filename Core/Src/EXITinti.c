/*
 * EXITinti.c
 *
 *  Created on: Dec 19, 2023
 *      Author: REDWOLF DiGiTAL
 */

#include "stm32f4xx.h"
#include "stm32f4xx_it.h"

unsigned char intterruptEvent_Flag = 0;

void initEXIT(void) {
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		// Enable GPIO Clock
	GPIOB -> MODER &= ~3U << 6;					// GPIO B3 set 00 - INPUT
	GPIOB -> PUPDR &= ~3U << 6;					// No pull-up|pull-down

	NVIC_EnableIRQ(EXTI3_IRQn);					// Enable interrupt

	RCC -> APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG -> EXTICR[0] &= SYSCFG_EXTICR1_EXTI3;
	SYSCFG -> EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PB;

	EXTI -> RTSR &= ~EXTI_RTSR_TR3;				// Disable rising trigger
	EXTI -> RTSR |= EXTI_FTSR_TR3;				// Enable falling trigger

	EXTI -> IMR |= EXTI_IMR_IM3;				// Interrupt Mask 1 = not mask
}


void EXTI3_IRQHandler(void) {
	if((EXTI -> PR & EXTI_PR_PR3) != 0) {
		intterruptEvent_Flag = 1;
		EXTI -> PR |= EXTI_PR_PR3;
	}
}

/*
 * dac.c
 *
 *  Created on: Aug 11, 2026
 *      Author: kdk78
 */


#include "dac.h"

void DAC_Init(void)
{
	// 1. DAC 클럭 활성화
	RCC->APB1ENR |= RCC_APB1ENR_DACEN;

	// 2. GPIOA 클럭 활성화(PA4, PA5 사용)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// PA4, PA5 아날로그 설정
	GPIOA->MODER &= ~((0x3<<GPIO_MODER_MODER4_Pos) | (0x3<<GPIO_MODER_MODER5_Pos ));
	GPIOA->MODER |= ((0x3<<GPIO_MODER_MODER4_Pos) | (0x3<<GPIO_MODER_MODER5_Pos));
	GPIOA->PUPDR &= ~((0x3U<<GPIO_PUPDR_PUPDR4_Pos) | (0x3U<<GPIO_PUPDR_PUPDR5_Pos));

	// DAc 채널 설정
	// 채널1 : PA4, 채널2: PA5
	// 트리거 사용x -> software mode, 내부 버퍼 사용
	// DAC->CR의 매크로 활용
	DAC->CR &= ~((DAC_CR_BOFF1 | DAC_CR_TEN1) |(DAC_CR_BOFF2 | DAC_CR_TEN2));
	DAC->CR |= (DAC_CR_EN1 | DAC_CR_EN2);
}

void DAC_SetValue_Ch1(uint16_t value)
{
	DAC->DHR12R1 = (value & 0xFFFU);
}

void DAC_SetValue_Ch2(uint16_t value)
{
	DAC->DHR12R2 = (value & 0xFFFU);
}



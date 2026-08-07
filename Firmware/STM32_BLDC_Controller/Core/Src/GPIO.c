/*
 * GPIO.c
 *
 *  Created on: Aug 7, 2026
 *      Author: kdk78
 */


#include "GPIO.h"

//PC6 핀 초기화 함수
void FLT_LED_Init(void)
{
	// 1. GPIOC 클럭 활성화
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

	// 2. GPIOC MODE 설정(MODER : 01)
	GPIOC->MODER |= (0x1U<<(6*2)); // PC6 출력 설정

	// 3. 출력 속도 및 풀업/풀다운 설정
	GPIOC->OSPEEDR |= (0x3<<(6*2)); // High Speed
}

//PC6 on/off 제어 함수
void FLT_LED_ONOFF(uint8_t on)
{
	if(on)
	{
		GPIOC->BSRR = GPIO_BSRR_BS6; // PC6 on : BSRR의 하위 16비트에 bit6을 쓰면 해당 핀 High
	}
	else
	{
		GPIOC->BSRR = GPIO_BSRR_BR6; // PC6 on : BSRR의 상위 16비트에 bit6을 쓰면 해당 핀 Low
	}
}

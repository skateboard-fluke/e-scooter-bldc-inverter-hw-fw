/*
 * Hall.c
 *
 *  Created on: Aug 7, 2026
 *      Author: kdk78
 */


#include "Hall.h"

void Initialize_Hall_Sensors(void)
{
	// 1. GPIOD clock 활성화
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

	// 2. PD0, PD1, PD2 입력 모드 설정 input mode:00
	GPIOD->MODER &= ~((0x3U<<(0*2)) | //PD0
					(0x3U<<(1*2))	| //PD1
					(0x3U<<(2*2)));	  //PD2
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // SYSCFG 클록 인가

	// EXTI0~2를 PD0~2로 매핑
	SYSCFG->EXTICR[0] &= ~(
							SYSCFG_EXTICR1_EXTI0_Msk |
							SYSCFG_EXTICR1_EXTI1_Msk |
							SYSCFG_EXTICR1_EXTI2_Msk);
	SYSCFG->EXTICR[0] |= (
							SYSCFG_EXTICR1_EXTI0_PD |
							SYSCFG_EXTICR1_EXTI1_PD |
							SYSCFG_EXTICR1_EXTI2_PD);

	//EXTI 라인 설정: IMR, RTSR, FTSR
	EXTI->IMR |= EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR2; //EXTI0,1,2 마스크 해제
	EXTI->RTSR |= EXTI_RTSR_TR0 | EXTI_RTSR_TR1 | EXTI_RTSR_TR2; // 상승 엣지 트리거
	EXTI->FTSR |= EXTI_FTSR_TR0 | EXTI_FTSR_TR1 | EXTI_FTSR_TR2; // 하강 엣지 트리거

	//NVIC에서 EXTI0, EXTI1, EXTI2 IRQ 활성화
	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI2_IRQn);

}


void Update_Swtiching_Pattern(uint8_t Hall_sum)
{

}





/*
 * timer.c
 *
 *  Created on: Aug 8, 2026
 *      Author: kdk78
 */

#include "timer.h"
#define DEADTIME_1us ((uint32_t)180)

void Initialize_PWM(void)/*Initialize TIM1 for PWM*/
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
	//PE13~8 -> TIM1 의 CH3/3N/2/2N/1/1N으로 사용
	GPIOE->MODER &=
				   ~(0x3<<(8*2) |
					0x3<<(9*2)  |
					0x3<<(10*2) |
					0x3<<(11*2) |
					0x3<<(12*2) |
					0x3<<(13*2));
	GPIOE->MODER |=
					((0x2<<(8*2)|
					0x2<<(9*2)  |
					0x2<<(10*2) |
					0x2<<(11*2) |
					0x2<<(12*2) |
					0x2<<(13*2)));
	GPIOE->AFR[1] &= ~(0xF<<(0*4) | 0xF<<(1*4) |0xF<<(2*4) |0xF<<(3*4) |0xF<<(4*4) |0xF<<(5*4));
	GPIOE->AFR[1] |= (0x1<<(0*4) | 0x1<<(1*4) |0x1<<(2*4) |0x1<<(3*4) |0x1<<(4*4) |0x1<<(5*4));

	GPIOE->OSPEEDR &= ~(0x3<<(8*2) | 0x3<<(9*2) | 0x3<<(10*2) | 0x3<<(11*2) | 0x3<<(12*2) | 0x3<<(13*2));
	GPIOE->OSPEEDR |= (0x3<<(8*2) | 0x3<<(9*2) | 0x3<<(10*2) | 0x3<<(11*2) | 0x3<<(12*2) | 0x3<<(13*2));


	// PWM signal = 180MHz
	RCC->APB2ENR |= 0x1; // TIM1 Clock Enable
	//216MHz

	TIM1->PSC = 0x0; // 216MHz/(0+1) = 216MHz
	TIM1->ARR = CNT_MAX;


	TIM1->CR2 &= ~(TIM_CR2_MMS_Msk);
	TIM1->CR2 |= (0x2U<<TIM_CR2_MMS_Pos);


	TIM1->CCMR1 =
				(0x1<<TIM_CCMR1_OC1PE_Pos) | // OC1PE
				(0x7<<TIM_CCMR1_OC1M_Pos) | // OC1M -> PWM MODE2
				(0x1<<TIM_CCMR1_OC2PE_Pos)| // OC2PE
				(0x7<<TIM_CCMR1_OC2M_Pos); // OC2M -> PWM MODE2
	TIM1->CCMR2 =
				(0x1<<TIM_CCMR2_OC3PE_Pos) | // OC3PE
				(0x7<<TIM_CCMR2_OC3M_Pos) ; // OC3M -> PWM MODE2


	// OC4M = PWM Mode 1 (110), OC4PE = 1
	TIM1->CCMR2 |= (0x6<<TIM_CCMR2_OC4M_Pos) | (0x1<<TIM_CCMR2_OC4PE_Pos);
	TIM1->CCR4 = 1;	// 바닥 직전(CNT=1)에서 트리거 펄스 출력
	TIM1->CCER |= TIM_CCER_CC4E;
	
	TIM1->CCER |=
				TIM_CCER_CC1E | // CC1E
	            TIM_CCER_CC1NE | // CC1NE
	            TIM_CCER_CC2E | // CC2E
	            TIM_CCER_CC2NE | // CC2NE
	            TIM_CCER_CC3NE | // CC3E
	            TIM_CCER_CC3NE; // CC3NE
	TIM1->BDTR =
				//TIM_BDTR_BKE | // BKE = 1
				TIM_BDTR_OSSR | // OSSR = 1
				TIM_BDTR_OSSI ; // OSSI = 1

	TIM1->BDTR |= TIM_BDTR_MOE | DEADTIME_1us;
	TIM1->DIER = 0x1; // enable update interrupt
	TIM1->CR1 =
				(0x3<<5) | //center-aligned mode3
				(0x1<<2) | //update request source
				(0x1<<0);  //counter enable

}



void Initialize_TIM2(void)/*Initialize TIM2 for Rpm Calculation*/
{
	//TIM2 clock 활성화
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;


	//TIM2 설정: 프리스케일러=1, 업카운터 모드, Auto-Reload 54,000,000
	/*
	 * TIM2 클록 설정 요약:
	 * - SYSCLK (HCLK) = 216MHz
	 * - APB1 클록 (PCLK1) = 216MHz / 4 = 54MHz
	 * - TIM2 내부 카운터 클록 = PCLK1 * 2 = 108MHz (APB1 분주비 != 1 일 때 자동 x2)
	 */
	TIM2->PSC =1; // 108MHz/(1+1) = 54MHz
	TIM2->ARR = 53999999; // Auto-Reload 54,000,000 -> 주기 = (54000+1)/(54MHz) = 1초
	TIM2->CNT = 0; // 초기 카운터 값

	// 타이머 시작
	TIM2->CR1 |= TIM_CR1_CEN;
}

void Enable_PWM(void)
{
	TIM1->BDTR |= TIM_BDTR_MOE; // MOE=1 Main output enable
}


void Disable_PWM(void)
{
	TIM1->BDTR &= ~(TIM_BDTR_MOE); //MOE = 0
	
}


void Start_TIM1_Control_Interrupt(void)
{
    // 1. TIM1 모듈의 Update Interrupt 활성화 (타이머 레벨)
    TIM1->DIER |= TIM_DIER_UIE;

    // 2. NVIC 인터럽트 활성화 (Cortex-M 코어 레벨)
    // CMSIS 표준 함수 사용 권장 (TIM1_UP_TIM10_IRQn 등 사용 칩셋에 맞는 번호)
    NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn); // 또는 NVIC->ISER[0] |= (1UL << 25);

    // [위험한 while문은 삭제합니다]
}

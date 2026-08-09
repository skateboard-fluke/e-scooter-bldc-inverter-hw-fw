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
	TIM1->CCMR1 =
				(0x1<<3) | // OC1PE
				(0x7<<4) | // OC1M -> PWM MODE2
				(0x1<<11)| // OC2PE
				(0x7<<12); // OC2M -> PWM MODE2
	TIM1->CCMR2 =
				(0x1<<3) | // OC3PE
				(0x7<<4) ; // OC3M -> PWM MODE2
	TIM1->CCER =
				(0x1<<0) | // CC1E
	            (0x1<<2) | // CC1NE
	            (0x1<<4) | // CC2E
	            (0x1<<6) | // CC2NE
	            (0x1<<8) | // CC3E
	            (0x1<<10); // CC3NE
	TIM1->BDTR =
				(0x1<<12) | // BKE = 1
				(0x1<<11) | // OSSR = 1
				(0x1<<10) ; // OSSI = 1

	TIM1->BDTR |= TIM_BDTR_MOE | DEADTIME_1us;
	TIM1->DIER = 0x1; // enable update interrupt
	TIM1->CR1 =
				(0x3<<5) | //center-aligned mode3
				(0x1<<2) | //update request source
				(0x1<<0);  //counter enable

}









void Initialize_TIM2(void)
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

void EXTI0_IRQHanlder(void)
{

}

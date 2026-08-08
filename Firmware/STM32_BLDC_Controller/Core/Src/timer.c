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


NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
while(!(TIM1->CR1 & 0x0010)); //TIM1 underflow event ?
TIM1->RCR = 0x0001; // 50 us period update(RCR =1)



void SysTick_Init(void)
{
	//SYSCLK가 216MHz일 떄, 1ms마다 인터럽트 발생
	//Reload = 216,000,000 / 1000 - 1 = 215999
	SysTick->LOAD = (216000000/1000)-1;
	SysTick->VAL = 0; // 현재 카운터 값 초기화
	SysTick->CTRL =
					SysTick_CTRL_CLKSOURCE_Msk |	// 프로세서 클록(216Mhz) 사용
					SysTick_CTRL_TICKINT_Msk |		// 인터럽트 활성화
					SysTick_CTRL_ENABLE_Msk;		// 카운터 Enable
}

void SysTick_Handler(void)
{
	msTicks++;
	Scheduler();
}


volatile uint32_t msTicks = 0;
static uint32_t lastTick_10ms = 0;
static uint32_t lastTick_100ms = 0;
static uint32_t lastTick_500ms = 0;
static uint32_t lastTick_1s = 0;


void Scheduler(void)
{
	// 1ms 태스크는 매번 실행
	Task_1ms();

	// 10ms 주기 태스크: 마지막 실행 시점으로부터 10ms 이상 경과 시 실행
	if((uint32_t)(msTicks-lastTick_10ms)>=10)
	{
		lastTick_10ms += 10;
		Task_10msFlg = 1;
	}

	// 100ms 주기 태스크: 마지막 실행 시점으로부터 100ms 이상 경과 시 실행
	if((uint32_t)(msTicks-lastTick_100ms)>=100)
	{
		lastTick_100ms += 100;
		Task_100msFlg = 1;
	}

	// 500ms 주기 태스크: 마지막 실행 시점으로부터 500ms 이상 경과 시 실행
	if((uint32_t)(msTicks-lastTick_500ms)>=500)
	{
		lastTick_500ms += 500;
		Task_500msFlg = 1;
	}

	// 1s 주기 태스크: 마지막 실행 시점으로부터 1s 이상 경과 시 실행
	if((uint32_t)(msTicks-lastTick_1s)>=1000)
	{
		lastTick_1s += 1000;
		Task_1sFlg = 1;
	}


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

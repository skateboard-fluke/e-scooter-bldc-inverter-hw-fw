#include "stm32f7xx.h"
#define CNT_MAX 5399 // 5400-1
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

void Scheduler(void)
{
	// 1ms 태스크는 매번 실행
	Task_1ms();

	// 10ms 주기 태스크: 1ms 카운터가 10의 배수이면 실행
	if((msTicks%10)==0)
	{
		Task_10msFlg = 1;
	}

	//100ms 주기 태스크: 1ms 카운터가 100의 배수이면 실행
	if((msTicks%100)==0)
	{
		Task_100msFlg = 1;
	}

	// 500ms 주기 태스크:1ms 카운터가 500의 배수이면 실행
	if((msTicks%500)==0)
	{
		Task_500msFlg = 1;
	}

	// 1초 주기 태스크:1ms 카운터가 1000의 배수이면 실행
	if((msTicks%1000)==0)
	{
		Task_1sFlg = 1;
	}


}








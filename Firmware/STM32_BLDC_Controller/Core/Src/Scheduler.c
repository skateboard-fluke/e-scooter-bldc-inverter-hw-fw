/*
 * Scheduler.c
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */


#include "Scheduler.h"

volatile uint32_t msTicks = 0;
static uint32_t lastTick_10ms = 0;
static uint32_t lastTick_100ms = 0;
static uint32_t lastTick_500ms = 0;
static uint32_t lastTick_1s = 0;

volatile uint8_t Task_10msFlg  = 0;
volatile uint8_t Task_100msFlg = 0;
volatile uint8_t Task_500msFlg = 0;
volatile uint8_t Task_1sFlg    = 0;



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



void Task_1ms(void)
{
	Motor_Control_PI_1ms();
}


void Task_10ms(void)
{
	System_Monitor_Check_10ms();
	UART2_Send_Rpm_Plot_10ms();
	Task_10msFlg = 0;
}

void Task_100ms(void)
{
	Task_100msFlg = 0;
}


void Task_500ms(void)
{
	Bluetooth_Send_Telemetry_500ms();
	Task_500msFlg = 0;
}

void Task_1s(void)
{
	Task_1sFlg=0;
}

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









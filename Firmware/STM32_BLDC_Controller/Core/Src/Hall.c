/*
 * Hall.c
 *
 *  Created on: Aug 7, 2026
 *      Author: kdk78
 */

#include "Hall.h"


uint32_t ccr_a =0;
uint32_t ccr_b =0;
uint32_t ccr_c =0;
unsigned int dir = 1;

uint8_t HA = 0;
uint8_t HB = 0;
uint8_t HC = 0;
uint8_t HallSum = 0;
uint8_t StartFlg = 0;  // UART에서 수신한 Start/Stop 명령 플래그
static const float Edges_per_Revolution = HALL_EDGES_PER_REV;

volatile uint32_t last_hall_cnt = 0;
volatile float calculated_rpm =0.0f;
volatile uint32_t delta_cnt =0;

volatile uint32_t hall_timeout_cnt=0;
	

void Initialize_Hall_Sensors(void)
{
	// 1. GPIOD clock 활성화
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

	// 2. PD0, PD1, PD2 입력 모드 설정 input mode:00
	GPIOD->MODER &= ~(
					(0x3U<<(0*2)) 	| //PD0
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


void Update_Switching_Pattern(uint8_t Hall_Sum)
{
	if(dir==1)//시계방향
	{
		switch(Hall_Sum)
		{
		case 6:
			// 상태 1
			Set_Phases(0, -1, 1);
			break;
		case 4:
			// 상태 2
			Set_Phases(-1, 0, 1);
			break;
		case 5:
			// 상태 3
			Set_Phases(-1, 1, 0);
			break;
		case 1:
			// 상태 4
			Set_Phases(0, 1, -1);
			break;
		case 3:
			// 상태 5
			Set_Phases(1, 0, -1);
			break;
		case 2:
			// 상태 6
			Set_Phases(1, -1, 0);
			break;
		default:
			// 모든 단계 비활성화
			Set_Phases(0,0,0);
			break;
		}

	}
	else
	{
		switch(Hall_Sum)//반시계 방향
		{
		case 5:
			// 상태 1: A 상승, B & C 낮음
			Set_Phases(0, 1, -1);
			break;
		case 3:
			// 상태 2: B 상승, A & C 낮음
			Set_Phases(1, -1, 0);
			break;
		case 1:
			// 상태 3: A 하강, B 상승, C 낮음
			Set_Phases(1, 0, -1);
			break;
		case 6:
			// 상태 4: C 상승, A & B 낮음
			Set_Phases(-1, 0, 1);
			break;
		case 4:
			// 상태 5: A 상승, B 낮음, C 하강
			Set_Phases(-1, 1, 0);
			break;
		case 2:
			// 상태 6: B 하강, A 낮음, C 상승
			Set_Phases(0, -1, 1);
			break;
		default:
			// 모든 단계 비활성화
			Set_Phases(0, 0, 0);
			break;
		}
	}
}


void Set_Phases(int32_t phaseA, int32_t phaseB, int32_t phaseC)
{
	//PhaseA 설정
	if(voltage_ref ==0 || MotorRunEnable==0)
	{
		Disable_PWM();
		return;
	}

	ccr_a = CNT_MAX - voltage_ref;
	ccr_b = CNT_MAX - voltage_ref;
	ccr_c = CNT_MAX - voltage_ref;

	//Phase A 설정
	if(phaseA==1) // PWM MODE 2
	{
		TIM1->CCR1 = ccr_a;
		TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk);
		TIM1->CCMR1 |= (0x7U<<TIM_CCMR1_OC1M_Pos);
	}else if(phaseA==-1)
	{
		TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk);
		TIM1->CCMR1 |= (0x5U<<TIM_CCMR1_OC1M_Pos);
	}
	else
	{
		TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk);
		TIM1->CCMR1 |= (0x4U<<TIM_CCMR1_OC1M_Pos);
	}

	//Phase B 설정
	if(phaseB == 1)
	{
		TIM1->CCR2 = ccr_b;
		TIM1->CCMR1 &= ~(TIM_CCMR1_OC2M_Msk);
		TIM1->CCMR1 |= (0x7U<<TIM_CCMR1_OC2M_Pos);
	}
	else if(phaseB == -1)
	{
		TIM1->CCMR1 &= ~(TIM_CCMR1_OC2M_Msk);
		TIM1->CCMR1 |= (0x5U<<TIM_CCMR1_OC2M_Pos);
	}
	else
	{
		TIM1->CCMR1 &= ~(TIM_CCMR1_OC2M_Msk);
		TIM1->CCMR1 |= (0x4U<<TIM_CCMR1_OC2M_Pos);
	}

	//Phase C 설정
	if(phaseC==1)
	{
		TIM1->CCR3 = ccr_c;
		TIM1->CCMR2	&= ~(TIM_CCMR2_OC3M_Msk);
		TIM1->CCMR2 |= (0x7U<<TIM_CCMR2_OC3M_Pos);
	}
	else if(phaseC == -1)
	{
		TIM1->CCMR2	&= ~(TIM_CCMR2_OC3M_Msk);
		TIM1->CCMR2 |= (0x5U<<TIM_CCMR2_OC3M_Pos);
	}
	else
	{
		TIM1->CCMR2	&= ~(TIM_CCMR2_OC3M_Msk);
		TIM1->CCMR2 |= (0x4U<<TIM_CCMR2_OC3M_Pos);
	}
}


void EXTI0_IRQHandler(void) //PD0 HA
{
	if(EXTI->PR & EXTI_PR_PR0)
	{
		EXTI->PR = EXTI_PR_PR0; //인터럽트 플래그 클리어
		Update_Hall_Sequence();
		SpeedCal();
	}
}

void EXTI1_IRQHandler(void) //PD1 HB
{
	if(EXTI->PR & EXTI_PR_PR1)
	{
		EXTI->PR = EXTI_PR_PR1; //인터럽트 플래그 클리어
		Update_Hall_Sequence();
		SpeedCal();
	}
}
void EXTI2_IRQHandler(void) //PD2 HC
{
	if(EXTI->PR & EXTI_PR_PR2)
	{
		EXTI->PR = EXTI_PR_PR2; //인터럽트 플래그 클리어
		Update_Hall_Sequence();
		SpeedCal();
	}
}

void Update_Hall_Sequence(void)
{
	// PD0, PD1, PD2에서 홀 신호 읽기
	HA = GPIOD->IDR & GPIO_IDR_IDR_0;
	HB = (GPIOD->IDR & GPIO_IDR_IDR_1)>>1;
	HC = (GPIOD->IDR & GPIO_IDR_IDR_2)>>2;
	HallSum = (HA<<2) | (HB<<1) | (HC);
}

void SpeedCal(void)
{
	// 현재 타이머 값 읽기
	volatile uint32_t current_cnt = TIM2->CNT; // current_cnt = 타이머의 현재 카운트

	// 타이머 오버플로우 처리
	if(current_cnt >= last_hall_cnt) // last_hall_cnt 지난 홀센서 이벤트의 카운트
	{
		delta_cnt = current_cnt - last_hall_cnt; // delta_cnt = 사이 간격 카운트
	}
	else
	{
		// 오버플로우 발생: 타이머가 ARR값에 도달하여 리셋됨
		delta_cnt = (TIM2->ARR - last_hall_cnt) + current_cnt +1;
	}


	if(delta_cnt <=500)
	{
		return;
	}

	last_hall_cnt = current_cnt;
	hall_timeout_cnt = 0;

	// 4. 속도 계산
    calculated_rpm = (60.0f * 54000000.0f) / ((float)Edges_per_Revolution * (float)delta_cnt);
	//delta_time 이 0이 아닐 때만 RPM 계산
	//가정 : 한 회전당 6개의 홀 센서 이벤트 발생 (6 edges per revolution)
	//Timer 주파수: 54MHz
	//RPM 계산 공식: RPM = (60*Clock_Frequency) / (Edges_per_Revolution * delta_time)
	

}







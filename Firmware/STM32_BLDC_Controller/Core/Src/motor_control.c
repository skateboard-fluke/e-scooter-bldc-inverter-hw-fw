/*
 * motor_control.c
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */


#include "motor_control.h"

float RpmRef,RpmErr,Pterm,Iterm,PIterm = 0.0f;
float Kp,Ki = 0.0f;
volatile float motor_speed_rpm = 0.0f;
uint8_t SpdFlg = 0;
volatile uint32_t voltage_ref=0;
float Vdc = 0.0f;
float MosfetTemp = 0.0f;
uint8_t FltFlg = 0;

float RpmNew =0.0f;
float RpmOld =0.0f;
uint32_t rpmHoldCounter	=0;
float ias, ibs, ics = 0.0f;
float ias_Cal, ibs_Cal, ics_Cal = 0.0f;
float iRamp = 0.15f;
float ias_LPF, ibs_LPF, ics_LPF = 0.0f;
uint32_t ias_Offset, ibs_Offset, ics_Offset = 0;

float Volt =0.0f;
float Throttle_ADC= 0.0f;
float I_MAX = 0.0f;
float ThrottleRef = 0.0f;
float I_Max = 0.0f;
float ThrottleRef_Ramp = 0.0f;

uint8_t FltCnt = 0;
uint8_t ThrottleActive = 0;
uint8_t InitCal = 0;
uint32_t Tim1TestCnt = 0;

float Fi = 0.0f;
float Ft = 0.0f;

#define MAX3(a,b,c) (((a)>(b)) ? (((a)>(c))?(a):(c)):(((b)>(c))?(b):(c)))
#define Tsamp	0.00005				// sampling time of current controller



void LPF(float input, float Fx, volatile float *output)
{
	*output = (1. - Fx)*(*output) + Fx*input;
}



void Motor_Init(void)
{
	Fi = 2.0f * PI * 500.0f * Tsamp / (1.0f + 2.0f * PI * 500.0f * Tsamp); // 500Hz
	Ft = 2.0f * PI * 1.0f   * Tsamp / (1.0f + 2.0f * PI * 1.0f   * Tsamp); // 1Hz
}

static inline void rampToTarget(float command, float *output, float slope)
{
	if(*output < command)
	{
		*output += slope;
		if(*output > command)
		{
			*output = command;
		}
	}
	else if(*output > command)
	{
		*output -= slope;
		if(*output < command)
		{
			*output = command;
		}
	}
}





void Motor_Control_PI_1ms(void)
{
	if(SpdFlg == 1)
	{
		RpmErr = RpmRef - motor_speed_rpm;
		Pterm = Kp*RpmErr;
		Iterm += Ki*RpmErr*0.001f; // 0.001--> 속도제어기가 실행되는 주기
		PIterm = Pterm + Iterm;

		if(PIterm > (float)(CNT_MAX-100))
		{
			PIterm = (float)(CNT_MAX-100);
		}
		voltage_ref = PIterm;
	}else
	{
		Pterm = 0.0f;
		Iterm = 0.0f;
		PIterm = 0.0f;
	}
}


/*
 * @brief		모터 정지 상태 감지 및 RPM 0.0f 강제 초기화
 * @details		모터 정지 시 홀 센서 EXTI 인터럽트 미발생으로 인해 calculated_rpm 변수가
 *          	마지막 속도 값에 고정(Stuck)되는 현상을 방지함.
 * @param		none
 * @return		void
 */

static inline void Motor_CheckRpmStuck(void)
{
	RpmNew = calculated_rpm;

	if(RpmNew == RpmOld)
	{
		rpmHoldCounter++;
		if(rpmHoldCounter>20000)
		{
			calculated_rpm = 0.0f;
			rpmHoldCounter=0;
		}
	}
	RpmOld = calculated_rpm;
}



static inline void Motor_ReadADC(void)
{
	uint32_t result = 0;

	// PA0(ias) 읽기 - ADC1
	ADC1->SQR3 = 0x00U; 				//SQ1 = 0(채널 0)
	ADC1->CR2 |= ADC_CR2_SWSTART; 		// SWSTART 비트 on
	while(!(ADC1->SR & ADC_SR_EOC)); 	//EOC대기
	result = ADC1->DR;					//변환 결과 읽기
	ias_Cal = ((float)(result - ias_Offset)*ADC_VREF/ADC_FS - OFFSET_Volt)/OPAMP_GAIN;
	ias = ias_Cal;
	LPF(ias, Fi, &ias_LPF);


	// PA1(ibs) 읽기 - ADC2
	ADC2->SQR3 = ADC_SQR3_SQ1_0; 		//SQ1 = 1(채널 1)
	ADC2->CR2 |= ADC_CR2_SWSTART; 		// SWSTART 비트 on
	while(!(ADC2->SR & ADC_SR_EOC)); 	//EOC대기
	result = ADC2->DR;					//변환 결과 읽기
	ibs_Cal = ((float)(result - ibs_Offset)*ADC_VREF/ADC_FS - OFFSET_Volt)/OPAMP_GAIN;
	ibs = ibs_Cal;
	LPF(ibs, Fi, &ibs_LPF);

	// PA2(ics) 읽기 - ADC3
	ADC3->SQR3 = ADC_SQR3_SQ1_1; 		//SQ1 = 2(채널 2)
	ADC3->CR2 |= ADC_CR2_SWSTART; 		// SWSTART 비트 on
	while(!(ADC3->SR & ADC_SR_EOC)); 	//EOC대기
	result = ADC3->DR;					//변환 결과 읽기
	ics_Cal = ((float)(result - ics_Offset)*ADC_VREF/ADC_FS - OFFSET_Volt)/OPAMP_GAIN;
	ics = ics_Cal;
	LPF(ics, Fi, &ics_LPF);

	// PA7(Throttle_ADC) 읽기 - ADC2
	ADC2->SQR3 = 0x7U<<ADC_SQR3_SQ1_Pos;
	ADC2->CR2 |= ADC_CR2_SWSTART;
	while(!(ADC2->SR & ADC_SR_EOC));
	result = ADC2->DR;
	Throttle_ADC = (float) result*3.3f/4095.0f;

}


static inline void Motor_CheckProtection(void)
{
	I_Max = MAX3(ias, ibs, ics);

	if(I_Max > OC_LEVEL)
	{
		FltCnt++;
		if(FltCnt >= 1000) // 50ms 동안 확인
		{
			FltFlg = 1;
			ThrottleRef = 0;
		}
	}
	else if(I_Max < 30.0f)
	{
		FltCnt = 0;
	}
}

/**
 * @brief 속도 LPF 및 km/h 단위 환산
 */

static inline void Motor_ProcessSpeed(void)
{
	LPF(calculated_rpm, Ft, &motor_speed_rpm);
}


/**
 * @brief 쓰로틀 히스테리시스 판단, 지령 맵핑 및 Ramp 제어
 */

static inline void Motor_ProcessThrottleCommand(void)
{
	// 히스테리시스를 주어 쓰로틀 신호 제어
	if(Throttle_ADC < THROTTLE_OFF)
	{
		ThrottleActive = 0;
	}
	else if(Throttle_ADC > THROTTLE_ON)
	{
		ThrottleActive = 1;
	}

	// 6-STEP 제어 모드
	if(!ThrottleActive) // 히스테리시스 값 이하
	{
		Disable_PWM();
		ThrottleRef = 0.0f;
		voltage_ref = 0;
	}
	else // 모터 구동
	{
		ThrottleRef = Throttle_ADC * 3400.0f - 3540.0f + 1000.0f; // 0826 듀티값 변경
	}

	rampToTarget(ThrottleRef, &ThrottleRef_Ramp, iRamp); //Ramp 함수를 통한 모터 응답성 조절

	if(SpdFlg == 0)
	{
		if(ThrottleRef_Ramp < 0.0f)
		{
			ThrottleRef_Ramp = 0.0f;
		}
		voltage_ref = (uint32_t) ThrottleRef_Ramp; // 형변환을 통한 최종 지령값 추출

		if(voltage_ref > CNT_MAX-100)
		{
			voltage_ref = CNT_MAX-100;
		}
	}


}

/**
 * @brief 6-Step 인버터 스위칭 패턴 출력 또는 비상 셧다운
 */
static inline void Motor_UpdateInverterOutput(void)
{
	if(FltFlg ==0 && InitCal ==1)
	{
		Update_Switching_Pattern(HallSum);
	}
	else
	{
		voltage_ref = 0;
		ccr_a = 0;
		ccr_b = 0;
		ccr_c = 0;
		//PWM 완전 차단
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
	}
}





/*
 * @brief		모터 정지 상태 감지 및 RPM 0.0f 강제 초기화
 * @details		모터 정지 시 홀 센서 EXTI 인터럽트 미발생으로 인해 calculated_rpm 변수가
 *          	마지막 속도 값에 고정(Stuck)되는 현상을 방지함.
 * @param		none
 * @return		void
 */

void TIM1_UP_TIM10_IRQHandler(void)
{
	Tim1TestCnt++;

	// 1. TIM10 예외 처리 복구 (공유 인터럽트 무한 재진입 방지)
	if (TIM10->SR & TIM_SR_UIF)
	{
		TIM10->SR &= ~TIM_SR_UIF;
	}

	// 2. TIM1 Update 인터럽트 처리
	if(TIM1->SR & TIM_SR_UIF)
	{
		TIM1->SR &= ~TIM_SR_UIF; // 진입 시점 플래그 클리어
		Motor_CheckRpmStuck();

		if(InitCal == 1)
		{
			Motor_ReadADC();
			Motor_CheckProtection();
			Motor_ProcessSpeed();
			Motor_ProcessThrottleCommand();
			Motor_UpdateInverterOutput();
		}
	}

}



void Motor_SetCurrentOffset(void)
{
	for(int i =0; i<10; i++)
	{
		// PA0(ias) 읽기 - ADC1
		ADC1->SQR3 = 0x00U; // SQ1 = 0 채널 0
		ADC1->CR2 |= ADC_CR2_SWSTART;
		while(!(ADC1->SR & ADC_SR_EOC));
		ias_Offset += ADC1->DR;

		// PA1(ibs) 읽기 - ADC2
		ADC2->SQR3 = ADC_SQR3_SQ1_0; // SQ1 = 0 채널 0
		ADC2->CR2 |= ADC_CR2_SWSTART;
		while(!(ADC2->SR & ADC_SR_EOC));
		ibs_Offset += ADC2->DR;

		// PA0(ics) 읽기 - ADC3
		ADC3->SQR3 = ADC_SQR3_SQ1_1; // SQ1 = 0 채널 0
		ADC3->CR2 |= ADC_CR2_SWSTART;
		while(!(ADC3->SR & ADC_SR_EOC));
		ics_Offset += ADC3->DR;

	}
	ias_Offset = (ias_Offset/10) - 2048;
	ibs_Offset = (ibs_Offset/10) - 2048;
	ics_Offset = (ics_Offset/10) - 2048;

	if(FltFlg==1)
	{
		InitCal=0;
	}
	else
	{
		InitCal = 1;
	}
}





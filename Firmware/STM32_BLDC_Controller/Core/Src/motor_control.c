/*
 * motor_control.c
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */


#include "motor_control.h"

#define RPM_EPSILON 0.01f // 노이즈 허용 오차 범위
float RpmRef =0.0f;
float RpmErr = 0.0f;
float Pterm = 0.0f;
float Iterm = 0.0f;
float PIterm = 0.0f;
float Kp = 0.2f;
float Ki = 0.0f;
volatile float motor_speed_rpm = 0.0f;
uint8_t MotorRunEnable = 1;
volatile uint32_t voltage_ref=0;
float Vdc = 0.0f;
float MosfetTemp = 0.0f;
uint8_t FltFlg = 0;

float RpmNew =0.0f;
float RpmOld =0.0f;
uint32_t rpmHoldCounter	=0;
float ias = 0.0f;
float ibs = 0.0f;
float ics = 0.0f;
float ias_Cal = 0.0f;
float ibs_Cal = 0.0f;
float ics_Cal = 0.0f;
float iRamp = 0.15f;
float ias_LPF = 0.0f;
float ibs_LPF = 0.0f;
float ics_LPF = 0.0f;
int32_t ias_Offset = 0;
int32_t ibs_Offset = 0;
int32_t ics_Offset = 0;

float Volt =0.0f;
float Throttle_ADC= 0.0f;
float ThrottleRef = 0.0f;
float I_Max = 0.0f;
float ThrottleRef_Ramp = 0.0f;

uint32_t FltCnt = 0;
uint8_t ThrottleActive = 0;
uint8_t InitCal = 1;
uint32_t Tim1TestCnt = 0;


uint8_t SpdFlg = 1;

float Fi = 0.0f;
float Ft = 0.0f;

#define Tsamp	0.00005				// sampling time of current controller



static inline float max3(float a, float b, float c)
{
	float m = (a > b) ? a: b;
	return (m > c) ? m : c;
}



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



/*
 * @brief 속도 PI 계산
*/

void Motor_Control_PI_1ms(void)
{
	if(!MotorRunEnable || !SpdFlg)
	{
		Pterm = 0.0f;
		Iterm = 0.0f;
		PIterm = 0.0f;
		return ;
	}

	const float dt = 0.001f; // 제어 주기
	const float out_max = (float) (CNT_MAX - 100);
	const float out_min = 0.0f;

	
	RpmErr = RpmRef - motor_speed_rpm;
	Pterm = Kp*RpmErr;

	/* 예측 적분(적분을 미리 계산) */
	float Iterm_next = Iterm + Ki*RpmErr*dt;
	float PI_unsat = Pterm + Iterm_next;

	/* 출력(clamp) 적용(0..out_max) */
	float PI_clamped = fminf(fmaxf(PI_unsat, out_min), out_max);

	/*
		방법 A: 조건부 적분(간단하고 안전)
		- PI_unsat가 클램프되지 않으면 적분 허용
		- PI_unsat가 클램프된 상태이면, 적분이 '포화를 더 악화시키는' 경우만 금지
		(즉, 포화 상태인데 에러가 포화를 더 밀어넣는 방향이면 적분하지 않음)
		*/
	if((PI_unsat == PI_clamped) || (PI_unsat > PI_clamped && RpmErr < 0.0f) || (PI_unsat < PI_clamped && RpmErr > 0.0f))
	{
		Iterm = Iterm_next; // 적분 허용
	}
	/* else: Iterm 유지(적분 동결) */

	/* 최종 PI와 안전한 클램프 및 무부호 대입 */

	PIterm = Pterm + Iterm;
	PIterm = fminf(fmaxf(PIterm, out_min), out_max);

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
	hall_timeout_cnt ++;

	// 50us * 4000 = 200ms (0.2초 동안 홀 신호가 없으면 모터 정지 판정)
    if(hall_timeout_cnt > 4000)
    {
        calculated_rpm = 0.0f;
        hall_timeout_cnt = 4000; // 카운터 오버플로우 방지
    }

}



static inline void Motor_ReadADC(void)
{

	// ★ while 대기 및 SWSTART 제거 (대기 시간 0)
	int32_t ias_raw = ADC1->JDR1;
	int32_t ibs_raw = ADC2->JDR1;
	int32_t ics_raw = ADC3->JDR1;

	// PA0(ias) 읽기 - ADC1
	ias_Cal = ((float)(ias_raw - ias_Offset)*ADC_VREF/ADC_FS - OFFSET_Volt)/OPAMP_GAIN;
	ias = ias_Cal;
	LPF(ias, Fi, &ias_LPF);


	// PA1(ibs) 읽기 - ADC2
	ibs_Cal = ((float)(ibs_raw - ibs_Offset)*ADC_VREF/ADC_FS - OFFSET_Volt)/OPAMP_GAIN;
	ibs = ibs_Cal;
	LPF(ibs, Fi, &ibs_LPF);

	// PA2(ics) 읽기 - ADC3
	
	ics_Cal = ((float)(ics_raw - ics_Offset)*ADC_VREF/ADC_FS - OFFSET_Volt)/OPAMP_GAIN;
	ics = ics_Cal;
	LPF(ics, Fi, &ics_LPF);
}


void Read_Throttle_10ms(void)
{
	
	ADC2->CR2 |= ADC_CR2_SWSTART;
	
	// 2. 변환 완료 대기 (main 루프에서 동작하므로 모터 제어에 영향 없음)
	while(!(ADC2->SR & ADC_SR_EOC));
	ADC2->SR &= ~ ADC_SR_EOC_Msk;

	// 3. DR에서 가져옴
	uint32_t throttle_raw = ADC2->DR;
	Throttle_ADC = (float) throttle_raw*3.3f/4095.0f;

}




static inline void Motor_CheckProtection(void)
{
	I_Max = max3(fabsf(ias), fabsf(ibs), fabsf(ics));

	if(I_Max > OC_LEVEL)
	{
		FltCnt++;
		if(FltCnt >= 3) // 0.15ms 동안 확인
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
	__disable_irq();
	float rpm_snapshot = calculated_rpm; // 홀 센서 속도 계산값 스냅샷
	__enable_irq();
	LPF(rpm_snapshot, Ft, &motor_speed_rpm);
}


/**
 * @brief 쓰로틀 히스테리시스 판단, 지령 맵핑 및 Ramp 제어
 * 쓰로틀 입력 -> ThrottleRef 계산
 */

static inline void Motor_ProcessThrottleCommand(void)
{
	// 히스테리시스를 주어 쓰로틀 신호 제어
	if(Throttle_ADC < THROTTLE_OFF_V)
	{
		ThrottleActive = 0;
	}
	else if(Throttle_ADC > THROTTLE_ON_V)
	{
		ThrottleActive = 1;
	}

	// 6-STEP 제어 모드
	if(!ThrottleActive) // 히스테리시스 값 이하
	{
		RpmRef = 0.0f;
		ThrottleRef = 0.0f;
	}
	else
	{
		float ratio = (Throttle_ADC - THROTTLE_OFF_V)/(THROTTLE_MAX_V - THROTTLE_OFF_V);

		if(ratio < 0.0f) ratio = 0.0f;
		if(ratio > 1.0f) ratio = 1.0f;
		if(SpdFlg == 1)
		{
			RpmRef = MIN_RUN_RPM + ratio*(MAX_RPM_TARGET - MIN_RUN_RPM);

		}
		else if(SpdFlg==0)
		{
			ThrottleRef =  DUTY_MIN_TARGET + ratio * (DUTY_MAX_TARGET - DUTY_MIN_TARGET);
		}
	}

	if(SpdFlg == 0)
	{
		rampToTarget(ThrottleRef, &ThrottleRef_Ramp, iRamp); //Ramp 함수를 통한 모터 응답성 조절
	}
	
}

/**
 * @brief 6-Step 인버터 스위칭 패턴 출력 또는 비상 셧다운
 */
void Motor_UpdateInverterOutput(void)
{
	if(FltFlg ==0 && InitCal ==1)
	{
		Update_Switching_Pattern(HallSum);
	}
	else
	{
		voltage_ref = 0;
		ccr_val = 0;
		//PWM 완전 차단
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
		Disable_PWM();
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
		}
	}

}



void Motor_SetCurrentOffset(void)
{

	ias_Offset = 0;
	ibs_Offset = 0;
	ics_Offset = 0;


	for(int i =0; i<10; i++)
	{
		// PA0(ias) 읽기 - ADC1
		ADC1->CR2 |= ADC_CR2_JSWSTART;
		while(!(ADC1->SR & ADC_SR_JEOC));
		ADC1->SR &= ~ADC_SR_JEOC_Msk;
		ias_Offset += ADC1->JDR1;

		// PA1(ibs) 읽기 - ADC2
		ADC2->CR2 |= ADC_CR2_JSWSTART;
		while(!(ADC2->SR & ADC_SR_JEOC));
		ADC2->SR &= ~ ADC_SR_JEOC_Msk;
		ibs_Offset += ADC2->JDR1;

		// PA0(ics) 읽기 - ADC3
		ADC3->CR2 |= ADC_CR2_JSWSTART;
		while(!(ADC3->SR & ADC_SR_JEOC));
		ADC3->SR &= ~ ADC_SR_JEOC_Msk;
		ics_Offset += ADC3->JDR1;

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
		// 오프셋 측정이 끝난 여기서 하드웨어 트리거(JEXTEN)를 활성화
        ADC1->CR2 |= (0x1U << ADC_CR2_JEXTEN_Pos);
        ADC2->CR2 |= (0x1U << ADC_CR2_JEXTEN_Pos);
        ADC3->CR2 |= (0x1U << ADC_CR2_JEXTEN_Pos);
	}
}



void Motor_UpdateControlOutput(void)
{
	if(FltFlg || !MotorRunEnable)
	{
		voltage_ref = 0;
		return;
	}

	if(SpdFlg == 1)
	{
		voltage_ref = (uint32_t)PIterm;
	}
	else
	{
		if(!ThrottleActive)
		{
			voltage_ref = 0;
			return;
		}

		if(ThrottleRef_Ramp < 0.0f)
		{
			ThrottleRef_Ramp = 0.0f;
		}

		voltage_ref = (uint32_t) ThrottleRef_Ramp;
	}

	if(voltage_ref > (CNT_MAX -100))
	{
		voltage_ref = CNT_MAX - 100;
	}

}

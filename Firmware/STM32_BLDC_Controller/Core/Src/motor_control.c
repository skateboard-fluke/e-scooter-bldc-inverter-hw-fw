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
volatile float speed_km_h = 0.0f;
float Vdc = 0.0f;
float MosfetTemp = 0.0f;
uint8_t FltFlg = 0;



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

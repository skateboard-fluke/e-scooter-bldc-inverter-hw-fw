/*
 * system_monitor.c
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */


#include "system_monitor.h"

float v_ntc = 0.0f;




void System_Monitor_Check_10ms(void)
{
	uint32_t result=0;
	// PA6(v_ntc) 읽기 - ADC1

	ADC1->SQR3 = (0x6U<<ADC_SQR3_SQ1_Pos)	;
	ADC1->CR2 |= ADC_CR2_SWSTART;
	//Regular channel end of conversion
	while(!(ADC1->SR & ADC_SR_EOC)); //EOC 대기 Regular channel end of conversion

	result = ADC1->DR;
	v_ntc = (float)result*ADC_VREF / ADC_FS;

	 //y = -11.489x3 + 63.236x2 - 149.02x + 181.97 NTC 3차 방정식
	 MosfetTemp = -11.48f*v_ntc*v_ntc*v_ntc+63.23*v_ntc*v_ntc-149.02*v_ntc+181.97f;

	 //PA3(Vdc) 일기 - ADC3
	 ADC3->SQR3 = 0x3U<<ADC_SQR3_SQ1_Pos;
	 ADC3->CR2 |= ADC_CR2_SWSTART;
	 while(!(ADC3->SR & ADC_SR_EOC));
	 result = ADC3->DR;
	 Vdc = (float)result*(ADC_VREF/ADC_FS) / VDIV_RATIO;


	 // 온도, 전압 기반 폴트 처리
	 if(Vdc < 32.0f)
	 {
		 FltFlg = 3; // 저전압 감지, 인휠모터 구동 시 주석 해제
	 }
	 else
	 {
		 //do nothing
	 }
	 if(MosfetTemp > 100.0f)
	 {
		 FltFlg = 2; // 과열 감지
	 }
	 else if(MosfetTemp < 90.0f && FltFlg==2)
	 {
		 FltFlg =0; // 온도가 정상이면 폴트 클리어
	 }
	 else
	 {
		 //do nothing
	 }


}

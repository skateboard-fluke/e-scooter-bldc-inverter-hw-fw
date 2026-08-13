/*
 * adc.c
 *
 *  Created on: Aug 8, 2026
 *      Author: kdk78
 */
#include "adc.h"

void Initialize_ADC(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // GPIOA 클럭 활성화
	// ADC 클럭 활성화
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN | RCC_APB2ENR_ADC3EN;
	// GPIO 초기화 PA0~7
	GPIOA->MODER &= ~(
					(0x3U << GPIO_MODER_MODER0_Pos) |
					(0x3U << GPIO_MODER_MODER1_Pos) |
					(0x3U << GPIO_MODER_MODER2_Pos) |
					(0x3U << GPIO_MODER_MODER3_Pos) |
					(0x3U << GPIO_MODER_MODER6_Pos) |
					(0x3U << GPIO_MODER_MODER7_Pos));


	GPIOA->MODER |= (
					(0x3U<<GPIO_MODER_MODER0_Pos) |
					(0x3U<<GPIO_MODER_MODER1_Pos) |
					(0x3U<<GPIO_MODER_MODER2_Pos) |
					(0x3U<<GPIO_MODER_MODER3_Pos) |
					(0x3U<<GPIO_MODER_MODER6_Pos) |
					(0x3U<<GPIO_MODER_MODER7_Pos));

	GPIOA->AFR[1] = 0x00U;
	GPIOA->AFR[0] = 0x00U;

	GPIOA->ODR = 0x00U;

	

	// ADC 공통 설정 - 독립 모드
	ADC->CCR &= ~ADC_CCR_MULTI; // 독립 모드. ADCCLK = PCLK2/2

	// ADC 샘플링 시간 설정(모든 채널 15 사이클)
	const uint32_t ADC_SMPR2_15CYCLES_CH0_7 = 0x00249249U;
	ADC1->SMPR2 = ADC_SMPR2_15CYCLES_CH0_7;
	ADC2->SMPR2 = ADC_SMPR2_15CYCLES_CH0_7;
	ADC3->SMPR2 = ADC_SMPR2_15CYCLES_CH0_7;

	// ADC1 설정 - PA0(IN0), PA6(IN6)
	ADC1->CR1 = 0x00U; // ADC analog input Channel 0
	ADC1->CR2 = 0x00U; //
	ADC1->SQR1 &= ~ ADC_SQR1_L; // 1개 변환 (L=0) -> PA0에서만 변환
	ADC1->SQR3 &= ~ ADC_SQR3_SQ1; // SQ1=0 (채널0)

	// ADC2 설정 - PA1(IN1), PA7(IN7)
	ADC2->CR1 = 0x00U;
	ADC2->CR2 = 0x00U;
	ADC2->SQR1 &= ~ ADC_SQR1_L; // 1개 변환 (L=0) -> PA1에서만 변환
	ADC2->SQR3 = (1U << ADC_SQR3_SQ1_Pos);

	// ADC3 설정 - PA2(IN2), PA3(IN3)
	ADC3->CR1 = 0x00U;
	ADC3->CR2 = 0x00U;
	ADC3->SQR1 &= ~ ADC_SQR1_L; // 1개 변환 (L=0) -> PA2에서만 변환
	ADC3->SQR3 = (2U << ADC_SQR3_SQ1_Pos);

	// ADC 활성화
	ADC1->CR2 |= ADC_CR2_ADON;
	ADC2->CR2 |= ADC_CR2_ADON;
	ADC3->CR2 |= ADC_CR2_ADON;

}









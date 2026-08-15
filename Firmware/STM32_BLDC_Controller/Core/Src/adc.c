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
	// GPIO 초기화 PA0~3, PA6,7
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

	// ---- ias (ADC1, PA0) : injected, TIM1 CC4 트리거 ----
	ADC1->CR1 &= ~ADC_CR1_AWDCH_Msk;
	ADC1->JSQR &= ~ADC_JSQR_JL_Msk;								//JL=0 , 1개 변환
	ADC1->JSQR |= (0x0U<<ADC_JSQR_JSQ4_Pos);					//JL=0 일때는 JSQ4에 채널 지정
	ADC1->CR2 &= ~(ADC_CR2_JEXTEN_Msk | ADC_CR2_JEXTSEL_Msk);
	ADC1->CR2 |= (0x1U<<ADC_CR2_JEXTSEL_Pos);// JEXTSEL=0001: TIM1 CC4

	// ---- ibs (ADC2, PA1) : injected, TIM1 CC4 트리거 ----
	ADC2->CR1 &= ~ADC_CR1_AWDCH_Msk;
	ADC2->JSQR &= ~ADC_JSQR_JL_Msk;								//JL=0 , 1개 변환
	ADC2->JSQR |= (0x1U<<ADC_JSQR_JSQ4_Pos);					//채널1 PA1
	ADC2->CR2 &= ~(ADC_CR2_JEXTEN_Msk | ADC_CR2_JEXTSEL_Msk);
	ADC2->CR2 |= (0x1U<<ADC_CR2_JEXTSEL_Pos);					// JEXTSEL=0001: TIM1 CC4

	// ---- ics (ADC3, PA2) : injected, TIM1 CC4 트리거 ----
	ADC3->CR1 &= ~ADC_CR1_AWDCH_Msk;
	ADC3->JSQR &= ~ADC_JSQR_JL_Msk;								//JL=0 , 1개 변환
	ADC3->JSQR |= (0x2U<<ADC_JSQR_JSQ4_Pos);					//채널2 PA2
	ADC3->CR2 &= ~(ADC_CR2_JEXTEN_Msk | ADC_CR2_JEXTSEL_Msk);
	ADC3->CR2 |= (0x1U<<ADC_CR2_JEXTSEL_Pos);					// JEXTSEL=0001: TIM1 CC4

	
	// ---- 쓰로틀 (ADC2, PA7) : regular, 10ms마다 소프트웨어 트리거 ----
	ADC2->CR2 &= ~(ADC_CR2_EXTEN_Msk | ADC_CR2_EXTSEL_Msk);
	ADC2->SQR1 &= ~ ADC_SQR1_L_Msk; 							// L=0, 1개 변환
	ADC2->SQR3 = (7U << ADC_SQR3_SQ1_Pos);   					//채널 7

	// ADC 활성화
	ADC1->CR2 |= ADC_CR2_ADON;
	ADC2->CR2 |= ADC_CR2_ADON;
	ADC3->CR2 |= ADC_CR2_ADON;

}









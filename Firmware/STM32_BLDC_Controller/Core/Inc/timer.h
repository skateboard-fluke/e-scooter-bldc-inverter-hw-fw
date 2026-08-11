/*
 * timer.h
 *
 *  Created on: Aug 8, 2026
 *      Author: kdk78
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

#include "stm32f7xx.h"
#include "Hall.h"
#define CNT_MAX 5399 // 5400-1
void Initialize_PWM(void);
void Initialize_TIM2(void);
void Enable_PWM(void);
void Disable_PWM(void);
void Start_TIM1_Control_Interrupt(void);
#endif /* INC_TIMER_H_ */

/*
 * Scheduler.h
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_
#include "stm32f7xx.h"
#include "motor_control.h"


void SysTick_Init(void);
void SysTick_Handler(void);
void Scheduler(void);
extern volatile uint8_t Task_10msFlg;
extern volatile uint8_t Task_100msFlg;
extern volatile uint8_t Task_500msFlg;
extern volatile uint8_t Task_1sFlg;
void Task_1ms(void);
extern volatile uint8_t Task_10msFlg;
extern volatile uint8_t Task_100msFlg;
extern volatile uint8_t Task_500msFlg;
extern volatile uint8_t Task_1sFlg;


#endif /* INC_SCHEDULER_H_ */

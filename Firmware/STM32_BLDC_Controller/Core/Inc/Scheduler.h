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
#include "uart.h"
#include "system_monitor.h"

void SysTick_Init(void);
void SysTick_Handler(void);
void Scheduler(void);
void Task_1ms(void);
void Task_10ms(void);
void Task_100ms(void);
void Task_500ms(void);
void Task_1s(void);
void Delay_ms(uint32_t ms);

extern volatile uint8_t Task_1msFlg;
extern volatile uint8_t Task_10msFlg;
extern volatile uint8_t Task_100msFlg;
extern volatile uint8_t Task_500msFlg;
extern volatile uint8_t Task_1sFlg;
extern volatile uint8_t Task_10msFlg;
extern volatile uint8_t Task_100msFlg;
extern volatile uint8_t Task_500msFlg;
extern volatile uint8_t Task_1sFlg;
extern volatile uint32_t msTicks;

#endif /* INC_SCHEDULER_H_ */

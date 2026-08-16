/*
 * Hall.h
 *
 *  Created on: Aug 7, 2026
 *      Author: kdk78
 */

#ifndef INC_HALL_H_
#define INC_HALL_H_

#include "stm32f7xx.h"
#include "timer.h"
#include "config.h"
#include "motor_control.h"

extern uint32_t ccr_val;
extern unsigned int dir;
extern uint8_t StartFlg;
extern volatile uint32_t voltage_ref;
extern volatile float calculated_rpm;
extern uint8_t HallSum;
extern volatile uint32_t hall_timeout_cnt;

void Initialize_Hall_Sensors(void);
void Update_Switching_Pattern(uint8_t Hall_sum);
void Set_Phases(int32_t phaseA, int32_t phaseB, int32_t phaseC);
void Update_Hall_Sequence(void);
void SpeedCal(void);

#endif /* INC_HALL_H_ */

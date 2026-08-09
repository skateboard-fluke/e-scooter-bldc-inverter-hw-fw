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

extern uint32_t ccr_a;
extern uint32_t ccr_b;
extern uint32_t ccr_c;
extern unsigned int dir;
extern uint8_t StartFlag;
extern volatile uint32_t voltage_ref;


void Initialize_Hall_Sensors(void);
void Update_Swtiching_Pattern(uint8_t Hall_sum);
void Set_Phases(int32_t phaseA, int32_t phaseB, int32_t phaseC);
void Mask_Channel(uint8_t channel);
void Unmask_Channel(uint8_t channel);
void Update_Hall_Sequence(void);
void SpeedCal(void);

#endif /* INC_HALL_H_ */

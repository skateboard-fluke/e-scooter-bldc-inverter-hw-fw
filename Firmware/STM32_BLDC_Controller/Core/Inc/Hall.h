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
void Initialize_Hall_Sensors(void);
void Update_Swtiching_Pattern(uint8_t Hall_sum);
void Set_Phases(int32_t phaseA, int32_t phaseB, int32_t phaseC);
void Mask_Channel(uint8_t channel);
void Unmask_Channel(uint8_t channel);
#endif /* INC_HALL_H_ */

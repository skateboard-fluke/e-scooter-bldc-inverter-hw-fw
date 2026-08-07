/*
 * Hall.h
 *
 *  Created on: Aug 7, 2026
 *      Author: kdk78
 */

#ifndef INC_HALL_H_
#define INC_HALL_H_

#include "stm32f7xx.h"
void Initialize_Hall_Sensors(void);
void Update_Swtiching_Pattern(uint8_t Hall_sum);
#endif /* INC_HALL_H_ */

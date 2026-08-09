/*
 * motor_control.h
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#include "stm32f7xx.h"
#include "timer.h"

extern float RpmRef;
extern float RpmErr;
extern float Pterm;
extern float Iterm;
extern float PIterm;
extern float Kp;
extern float Ki;
extern volatile float motor_speed_rpm;
extern uint8_t SpdFlg;
extern volatile uint32_t voltage_ref;
extern volatile float speed_km_h;
extern float Vdc;
extern float MosfetTemp;
extern uint8_t FltFlg;


void Motor_Control_PI_1ms(void);
#endif /* INC_MOTOR_CONTROL_H_ */

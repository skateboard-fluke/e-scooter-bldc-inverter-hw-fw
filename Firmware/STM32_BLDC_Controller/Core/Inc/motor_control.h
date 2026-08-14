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
#include "Hall.h"
#include "config.h"
#include "math.h"

extern float RpmRef;
extern float RpmErr;
extern float Pterm;
extern float Iterm;
extern float PIterm;
extern float Kp;
extern float Ki;
extern volatile float motor_speed_rpm;
extern volatile uint32_t voltage_ref;
extern float Vdc;
extern float MosfetTemp;
extern uint8_t FltFlg;
extern uint8_t MotorRunEnable;
extern uint8_t SpdFlg;

void LPF(float input, float Fx, volatile float *output);
void Motor_Init(void);
void Motor_Control_PI_1ms(void);
void Motor_SetCurrentOffset(void);
void Motor_UpdateControlOutput(void);
void Motor_UpdateInverterOutput(void);
void Read_Throttle_10ms(void);

#endif /* INC_MOTOR_CONTROL_H_ */

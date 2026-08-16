/*
 * uart.h
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */

#ifndef INC_UART_H_
#define INC_UART_H_
#include "stm32f7xx.h"
#include "motor_control.h"
#include "Scheduler.h"

extern volatile char usart2_rx_buf[64];
extern volatile uint8_t USART2_CmdReadyFlg;

extern volatile char usart3_rx_buf[64];
extern volatile uint8_t USART3_CmdReadyFlg;



void AT09_Init(void);
void USART3_SendChar(char c);
char USART3_ReceiveChar(void);
void UART3_SendFloat_Simple(float value, int decimals);
void Bluetooth_Send_Telemetry_500ms(void);
void USART2_Init(void);
void USART2_SendChar(char c);
void USART2_SendString(const char*str);
char USART2_ReceiveChar(void);
void USART2_SendFloat_Simple(float value, int decimals);
void USART2_Send_Rpm_Plot_10ms(void);


#endif /* INC_UART_H_ */

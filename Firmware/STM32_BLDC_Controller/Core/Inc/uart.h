/*
 * uart.h
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */

#ifndef INC_UART_H_
#define INC_UART_H_
#include "stm32f7xx.h"

void AT09_Init(void);
void USART3_SendChar(char c);
char USART3_ReceiveChar(void);
void UART3_SendFloat_Simple(float value, int decimals);

#endif /* INC_UART_H_ */

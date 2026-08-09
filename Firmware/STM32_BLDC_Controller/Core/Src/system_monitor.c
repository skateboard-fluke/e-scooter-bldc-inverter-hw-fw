/*
 * system_monitor.c
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */


#include "system_monitor.h"


void System_Monitor_Check_10ms(void)
{
	uint32_t result=0;
	// PA6(tempLaw) 읽기 - ADC1

	ADC1->SQR3 = 0x6U;
	ADC1->CR2 |= ADC_CR2_SWSTART;


}

/*
 * uart.c
 *
 *  Created on: Aug 9, 2026
 *      Author: kdk78
 */
#include "uart.h"


void AT09_Init(void) // BT
{
	// UART3 클럭 활성화
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

	// GPIOD 클럭 활성화
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

	// PD8(TX) PD9(RC) - > Alternate Function AF7
	GPIOD->MODER &= ~(0x3U<<GPIO_MODER_MODER8_Pos);
	GPIOD->MODER &= ~(0x3U<<GPIO_MODER_MODER9_Pos);
	GPIOD->MODER |= 0x2U<<GPIO_MODER_MODER8_Pos; // AF mode
	GPIOD->MODER |= 0x2U<<GPIO_MODER_MODER9_Pos; // AF mode


	GPIOD->AFR[1] &= ~(0xFU<<GPIO_AFRH_AFRH0_Pos);
	GPIOD->AFR[1] &= ~(0xFU<<GPIO_AFRH_AFRH1_Pos);
	GPIOD->AFR[1] |= 0x7U<<GPIO_AFRH_AFRH0_Pos;
	GPIOD->AFR[1] |= 0x7U<<GPIO_AFRH_AFRH1_Pos;

	// USART3 레지스터 초기화
	USART3->CR1 = 0;
	USART3->CR2 = 0;
	USART3->CR3 = 0;

	// Baud Rate 9600bps @ APB1=54MHz -> BRR = 5625
	USART3->BRR = 5625;

	// TE=1 RE=1 UE=1
	USART3->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
	// TE : Transmitter Enable
	// RE : Receive Enable
	// UE : USART Enable

}

uint32_t USART_Timeout = 0;
void USART3_SendChar(char c)
{
	// TXE=1 (전송 데이터 레지스터 비어있음) 대기
	while(!(USART3->ISR & USART_ISR_TXE));
	USART3->TDR = (uint8_t) c;
}

void USART3_SendString(const char *str)
{
	while(*str)
	{
		USART3_SendChar(*str++);
	}
}



char USART3_ReceiveChar(void)
{
	// RXNE=1 될때 까지 대기
	while(!(USART3->ISR & USART_ISR_RXNE))
	{
		if(USART_Timeout>5000)
		{
			USART_Timeout = 0;

			break;
		}
		else
		{
			USART_Timeout++;
		}
	}
	return (char)(USART3->RDR & 0xFF);
}


void USART3_SendFloat_Simple(float value, int decimals)
{
    // 1) 부호 처리
    if (value < 0.0f)
    {
        USART3_SendChar('-');
        value = -value;  // 양수화
    }

    // 2) 자릿수에 맞춘 반올림 보정 (소수점 버림 오차 방지)
    // 예: decimals가 2이면 0.005를 더해 반올림 처리
    float rounding = 0.5f;
    for (int i = 0; i < decimals; i++)
    {
        rounding /= 10.0f;
    }
    value += rounding;

    // 3) 정수부 / 소수부 분리
    uint32_t iPart = (uint32_t)value;
    float frac = value - (float)iPart;

    // 4) 정수부 변환 및 전송
    char tmpBuf[12];
    int idx = 0;

    if (iPart == 0)
    {
        USART3_SendChar('0');
    }
    else
    {
        while (iPart > 0 && idx < (int)(sizeof(tmpBuf) - 1))
        {
            tmpBuf[idx++] = (char)('0' + (iPart % 10));
            iPart /= 10;
        }

        // 역순 출력
        while (idx > 0)
        {
            USART3_SendChar(tmpBuf[--idx]);
        }
    }

    // 5) 소수부 변환 및 전송
    if (decimals > 0)
    {
        USART3_SendChar('.');  // 소수점 출력

        for (int i = 0; i < decimals; i++)
        {
            frac *= 10.0f;
            int digit = (int)frac; // 0 ~ 9
            USART3_SendChar((char)('0' + digit));
            frac -= (float)digit;  // 소수점 이하 갱신
        }
    }
}


void Bluetooth_Send_Telemetry_500ms(void)
{
	// 블루투스 송신
	USART3_SendString("Spd :");
	USART3_SendFloat_Simple(speed_km_h, 1);
	USART3_SendString("\n");

	USART3_SendString("Vdc :");
	USART3_SendFloat_Simple(Vdc, 1);
	USART3_SendString("\n");

	USART3_SendString("MosfetTemp :");
	USART3_SendFloat_Simple(MosfetTemp, 1);
	USART3_SendString("\n");

	USART3_SendString("Flt :");
	USART3_SendChar(FltFlg+48);
	USART3_SendString("\n");
}


void USART2_init(void)
{
	// USART2 클럭 활성화, GPIOD 클럭 활성화
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

	// PD5,6 Mode = AF7
	GPIOD->MODER &= ~(0x3<<GPIO_MODER_MODER5_Pos | 0x3<<GPIO_MODER_MODER6_Pos);
	GPIOD->MODER |= (0x2<<GPIO_MODER_MODER5_Pos | 0x2<<GPIO_MODER_MODER6_Pos);

	GPIOD->AFR[0] &= ~(0xF<<GPIO_AFRL_AFRL5_Pos |0xF<<GPIO_AFRL_AFRL6_Pos);
	GPIOD->AFR[0] |= (0x7<<GPIO_AFRL_AFRL5_Pos |0x7<<GPIO_AFRL_AFRL6_Pos);

	// USART2 레지스터 초기화
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = 0;

	// Baud Rate 9600bps @ APB1=54MHz -> BRR = 5625
	USART2->BRR = 5625;

	// TE=1 RE=1 UE=1
	USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
	// TE : Transmitter Enable
	// RE : Receive Enable
	// UE : USART Enable

}

void USART2_SendChar(char c)
{
	// TXE=1 (전송 데이터 레지스터 비어있음) 대기
	while(!(USART2->ISR &USART_ISR_TXE));
	USART2->TDR = (uint8_t)c;
}


void USART2_SendString(const char*str)
{
	while(*str)
	{
		USART2_SendChar(*str++);
	}
}


char USART2_ReceiveChar(void)
{
	// RXNE=1 까지 대기
	while(!(USART2->ISR & USART_ISR_RXNE))
	{
		if(USART_Timeout>5000)
		{
			USART_Timeout = 0;
			break;
		}
		else
		{
			USART_Timeout++;
		}
	}
	return (char) (USART2->RDR & 0xFF);
}

void USART2_SendFloat_Simple(float value, int decimals)
{
    // 1) 부호 처리
    if (value < 0.0f)
    {
        USART2_SendChar('-');
        value = -value;  // 양수화
    }

    // 2) 자릿수에 맞춘 반올림 보정 (소수점 버림 오차 방지)
    // 예: decimals가 2이면 0.005를 더해 반올림 처리
    float rounding = 0.5f;
    for (int i = 0; i < decimals; i++)
    {
        rounding /= 10.0f;
    }
    value += rounding;

    // 3) 정수부 / 소수부 분리
    uint32_t iPart = (uint32_t)value;
    float frac = value - (float)iPart;

    // 4) 정수부 변환 및 전송
    char tmpBuf[12];
    int idx = 0;

    if (iPart == 0)
    {
        USART2_SendChar('0');
    }
    else
    {
        while (iPart > 0 && idx < (int)(sizeof(tmpBuf) - 1))
        {
            tmpBuf[idx++] = (char)('0' + (iPart % 10));
            iPart /= 10;
        }

        // 역순 출력
        while (idx > 0)
        {
            USART2_SendChar(tmpBuf[--idx]);
        }
    }

    // 5) 소수부 변환 및 전송
    if (decimals > 0)
    {
        USART2_SendChar('.');  // 소수점 출력

        for (int i = 0; i < decimals; i++)
        {
            frac *= 10.0f;
            int digit = (int)frac; // 0 ~ 9
            USART2_SendChar((char)('0' + digit));
            frac -= (float)digit;  // 소수점 이하 갱신
        }
    }
}


void UART2_Send_Rpm_Plot_10ms(void)
{
	USART2_SendString(">RpmRef:");
	USART2_SendFloat_Simple(RpmRef, 1);
	USART2_SendString("\n");

	USART2_SendString(">RpmFdb:");
	USART2_SendFloat_Simple(motor_speed_rpm, 1);
	USART2_SendString("\n");
}




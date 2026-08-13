#include "stm32f7xx.h"
#include "clock.h"
#include "GPIO.h"
#include "adc.h"
#include "timer.h"
#include "Hall.h"
#include "uart.h"
#include "Scheduler.h"
#include "dac.h"

uint8_t init_drive = 0;

int main(void)
{
    // 1. 하드웨어 및 페리페럴 초기화
    Initialize_MCU();
    Initialize_ADC();
    Initialize_PWM();
    Initialize_TIM2();
    FLT_LED_Init();
    Initialize_Hall_Sensors();
    AT09_Init();
    SysTick_Init();
    USART2_Init();
    Motor_Init();
    DAC_Init();

    // 2. 센서 오프셋 설정 및 제어 인터럽트 개시
    Motor_SetCurrentOffset();
    Start_TIM1_Control_Interrupt();
    Update_Hall_Sequence();

    uint8_t prev_StartFlg = 0;

    while(1)
    {
        // 백그라운드 타이머 카운팅 스케줄러
        Scheduler();

        // 각 백그라운드 태스크 독립 실행 (if - else if 제거)
        if (Task_1msFlg)   { Task_1msFlg = 0; Task_1ms(); }
        if (Task_10msFlg)  { Task_10msFlg = 0; Task_10ms(); }
        if (Task_100msFlg) { Task_100msFlg = 0; Task_100ms(); }
        if (Task_500msFlg) { Task_500msFlg = 0; Task_500ms(); }
        if (Task_1sFlg)    { Task_1sFlg = 0; Task_1s(); }

        
        if(USART2_CmdReadyFlg)
        {
            USART2_CmdReadyFlg = 0;
            if(strcmp((char*)(usart2_rx_buf), "Start")==0)
            {
                StartFlg = 1;
                USART2_SendString("PC: Start Command Received!\r\n");
            }
            else if(strcmp((char*)usart2_rx_buf, "Stop")==0)
            {
                StartFlg = 0;
                USART2_SendString("PC: Motor Stop!\r\n ");
            }
            else if(strcmp((char*)usart2_rx_buf, "SpeedMode")==0)
            {
                if(StartFlg == 1)
                {
                    SpdFlg = 1;
                    USART2_SendString("PC: Speed Mode!\r\n");
                }
                else
                {
                    USART2_SendString("PC: Start motor first!\r\n");
                }

            }
        }

    
        // 모터 구동 상태 변경 감지 (엣지 트리거 방식)
        if (StartFlg != prev_StartFlg)
        {
            if (StartFlg == 1)
            {
                MotorRunEnable = 1;
                Enable_PWM();
            }
            else
            {
                MotorRunEnable = 0;
                SpdFlg = 0;
                Disable_PWM();
            }
            prev_StartFlg = StartFlg;
        }

       
        
    }
}

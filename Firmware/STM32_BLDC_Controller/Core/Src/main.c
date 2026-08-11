#include "stm32f7xx.h"
#include "clock.h"
#include "GPIO.h"
#include "adc.h"
#include "timer.h"
#include "hall.h"
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

    uint8_t prev_StartFlag = 0;

    while(1)
    {
        // 백그라운드 타이머 카운팅 스케줄러
        Scheduler();

        // 각 백그라운드 태스크 독립 실행 (if - else if 제거)
        if (Task_10msFlg)
        {
            Task_10msFlg = 0; // 플래그 클리어 위치 확인 필요
            Task_10ms();
        }

        if (Task_100msFlg)
        {
            Task_100msFlg = 0;
            Task_100ms();
        }

        if (Task_500msFlg)
        {
            Task_500msFlg = 0;
            Task_500ms();
        }

        if (Task_1sFlg)
        {
            Task_1sFlg = 0;
            Task_1s();
        }

        // 모터 구동 상태 변경 감지 (엣지 트리거 방식)
        if (StartFlag != prev_StartFlag)
        {
            if (StartFlag == 1)
            {
                Enable_PWM();
                init_drive = 1;
            }
            else
            {
                Disable_PWM();
                init_drive = 0;
            }
            prev_StartFlag = StartFlag;
        }
    }
}

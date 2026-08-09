#include "stm32f7xx.h"
#include "clock.h"
#include "GPIO.h"
#include "adc.h"
#include "timer.h"
#include "hall.h"
#include "uart.h"
#include "Scheduler.h"










int main(void)
{
	while(1)
	{
		// 1. 백그라운드에서 스케줄러를 계속 돌리며 시간 체크
		Scheduler();

		// 2. 깃발이 올라오면 해당 태스크 수행 후 깃발 내리기
		if(Task_10msFlg)
		{
			Task_10msFlg = 0;
			// 10ms 태스크 실행
		}
		if(Task_100msFlg)
		{
			Task_100msFlg = 0;
			// 100ms 태스크 실행
		}
		if(Task_500msFlg)
		{
			Task_500msFlg = 0;
			// 500ms 태스크 실행 (예: NTC 온도 읽기)
		}
		if(Task_1sFlg)
		{
			Task_1sFlg = 0;
			// 1s 태스크 실행 (예: 블루투스로 속도 전송)
		}

	}
}





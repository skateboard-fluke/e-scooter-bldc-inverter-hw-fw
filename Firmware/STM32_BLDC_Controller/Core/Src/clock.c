#include "clock.h"

void Initialize_MCU(void)
{
	// 1. 명령 캐시 및 데이터 캐시 설정
	SCB_EnableICache();
	SCB_EnableDCache();

	// 2. ART 가속기, 프리패치 버퍼, 웨이트 사이클 설정
	FLASH->ACR =
				FLASH_ACR_ARTEN_Msk |
				FLASH_ACR_PRFTEN_Msk |
				FLASH_ACR_LATENCY_7WS;


	// 3. HSE 및 PLL 설정 SYSCLk = 216MHz
	RCC->CR |= (RCC_CR_HSEON |RCC_CR_HSION); // HSE, HSI 모두 On
	while((RCC->CR & RCC_CR_HSIRDY)==0); // HSIRDY가 1이 될때까지 대기
	while ((RCC->CR & RCC_CR_HSERDY) == 0); // HSERDY가 1 될때까지 대기

	RCC->CFGR = RCC_CFGR_SW_HSI;			// 임시로 HSI를 SYSCLK으로 사용
	while (RCC->CFGR & RCC_CFGR_SWS);	// HSI가 SYSCLK이 될때 까지 대기 / SW: System Clock, SWS: status

	RCC->CR |= (RCC_CR_HSEON |RCC_CR_HSION);  // HSE On, HSI On, PLL off
	// SYSCLK = HSE*PLLN/PLLM/PLLP = 16Mhz*216/8/2 = 216MHz
	RCC->PLLCFGR =
					(216U<<RCC_PLLCFGR_PLLN_Pos) | //PLLN = 216
					(8U<<RCC_PLLCFGR_PLLM_Pos)   | //PLLM = 8
					(0U<<RCC_PLLCFGR_PLLP_Pos)   | //PLLP = 2
					RCC_PLLCFGR_PLLSRC_HSE;			// HSE ON

	RCC->CR |= RCC_CR_PLLON | RCC_CR_HSEON | RCC_CR_HSION; //PLL On, HSE On, HSI On
	while((RCC->CR & RCC_CR_PLLRDY)==0);	// PLLRDY=1 까지 대기

	// 4. 오버 드라이브 설정
	RCC->APB1ENR |= RCC_APB1ENR_PWREN; // 전원 모듈 clock (PWREN =1)
	PWR->CR1 |= PWR_CR1_ODEN;			// PWR의 Overdrive Enable 활성화
	while((PWR->CSR1 & PWR_CSR1_ODRDY)==0); // ODRDY = 1 까지 대기
	PWR->CR1 |= PWR_CR1_ODSWEN;			// PWR 의 over-drive switching enable
	while((PWR->CSR1 & PWR_CSR1_ODSWRDY)==0); // ODSWRDY = 1까지 대기

	// 5. 주변장치 클록 설정
	RCC->CFGR =
				RCC_CFGR_SW_PLL | 			// System Clock PLL사용 / AHB = 216MHz
				RCC_CFGR_PPRE1_DIV4 |  		// APB1 = 54MHz
				RCC_CFGR_PPRE2_DIV4 ;		// APB2 = 54MHz
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // SYSCLk = PLL 까지 대기
	RCC->CR |= RCC_CR_CSSON;




}

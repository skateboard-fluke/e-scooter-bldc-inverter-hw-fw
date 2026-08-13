# 1ms 작업 vs 인터럽트 분리 분석

## 📋 시스템 개요

### 실행 흐름
```
┌─────────────────────────────────────────────────────┐
│             Main Loop (Non-Interrupt)               │
│  - Scheduler()                                      │
│    └─> Task_1ms() ◄─── **1ms 주기 작업**           │
│        ├─ Motor_Control_PI_1ms()                   │
│        ├─ Motor_UpdateControlOutput()              │
│        └─ Motor_UpdateInverterOutput()             │
│    └─> Task_10ms() / Task_100ms() / ...            │
└─────────────────────────────────────────────────────┘
         ▲
         │
    ┌────┴────────────────────────────────────────────┐
    │         Interrupt Handlers                      │
    │                                                  │
    ├─ SysTick_Handler()  ◄─── **1ms 타이머**        │
    │  └─> msTicks++                                 │
    │                                                  │
    ├─ TIM1_UP_TIM10_IRQHandler() ◄─ **1ms 제어기** │
    │  └─> Motor_CheckRpmStuck()                    │
    │  └─> Motor_ReadADC()  ⚠️ 공유 변수              │
    │  └─> Motor_CheckProtection()  ⚠️ 공유 변수     │
    │  └─> Motor_ProcessSpeed()  ⚠️ 공유 변수        │
    │  └─> Motor_ProcessThrottleCommand()  ⚠️ 공유   │
    │                                                  │
    ├─ EXTI0/1/2_IRQHandler() ◄─ **홀 센서**        │
    │  └─> Update_Hall_Sequence()  ⚠️ 공유 변수      │
    │  └─> SpeedCal()  ⚠️ 공유 변수                 │
    └────────────────────────────────────────────────┘
```

---

## 🚨 주요 문제점 및 위험 요소

### **1. 핵심 문제: 다중 인터럽트에서 공유 변수 접근**

#### **문제 1-1: `calculated_rpm` 데이터 경쟁 (Data Race)**

| 코드 위치 | 접근 방식 | 기능 |
|---------|---------|------|
| **Hall.c - EXTI0/1/2_IRQHandler** | **쓰기** (Write) | `SpeedCal()` 함수에서 값 갱신 |
| **motor_control.c - TIM1_UP_TIM10_IRQHandler** | **읽기** (Read) | `Motor_ProcessSpeed()`에서 읽음 |
| **Scheduler.c - Task_1ms()** | **읽기** (Read) | `Motor_Control_PI_1ms()`에서 사용 |
| **motor_control.c - Motor_CheckRpmStuck()** | **읽기** (Read) | RPM 정지 감지용 |

```c
// ⚠️ EXTI 인터럽트에서 계속 업데이트
void SpeedCal(void)
{
    // ...
    calculated_rpm = (60* 54000000.0f)/(Edges_per_Revolution * (float)(delta_cnt));
    // ↓ 동시 접근 위험!
}

// ⚠️ TIM1 인터럽트 & 메인 루프에서 읽음
void Motor_ProcessSpeed(void)
{
    LPF(calculated_rpm, Ft, &motor_speed_rpm);  // 여기서 읽음
}

void Motor_CheckRpmStuck(void)
{
    RpmNew = calculated_rpm;  // 여기서도 읽음
}
```

**위험 수준**: 🔴 **높음** - 홀 센서가 회전하는 동안 지속적으로 인터럽트 발생
- 홀 센서 이벤트: 모터 회전당 6개 발생 (회전 속도에 따라 µs ~ ms 단위)
- TIM1 인터럽트: 1ms 주기
- 동시 접근 가능성: **매우 높음** ⚠️

---

#### **문제 1-2: `HallSum` 데이터 경쟁**

```c
// ⚠️ EXTI 인터럽트에서 쓰기
void Update_Hall_Sequence(void)
{
    HA = GPIOD->IDR & GPIO_IDR_IDR_0;
    HB = (GPIOD->IDR & GPIO_IDR_IDR_1)>>1;
    HC = (GPIOD->IDR & GPIO_IDR_IDR_2)>>2;
    HallSum = (HA<<2) | (HB<<1) | (HC);
    // ↓ 동시 접근 위험!
}

// ⚠️ TIM1 인터럽트 & main 루프에서 읽음
void Motor_UpdateInverterOutput(void)
{
    if(FltFlg ==0 && InitCal ==1)
    {
        Update_Switching_Pattern(HallSum);  // 여기서 읽음
    }
}
```

**위험 수준**: 🔴 **매우 높음** - 잘못된 HallSum 값 → 잘못된 스위칭 패턴 → 모터 제어 오류

---

#### **문제 1-3: 전류 측정값 (`ias`, `ibs`, `ics`) 데이터 경쟁**

```c
// ⚠️ TIM1 인터럽트에서 쓰기
void Motor_ReadADC(void)
{
    // ...
    ias = ias_Cal;
    LPF(ias, Fi, &ias_LPF);
    
    ibs = ibs_Cal;
    LPF(ibs, Fi, &ibs_LPF);
    
    ics = ics_Cal;
    LPF(ics, Fi, &ics_LPF);
}

// ⚠️ TIM1 인터럽트 같은 시간대에 사용
void Motor_CheckProtection(void)
{
    I_Max = max3(ias, ibs, ics);  // 불완전하게 업데이트된 값 읽을 위험
}
```

**위험 수준**: 🔴 **높음** - 불완전하게 업데이트된 전류값으로 잘못된 보호 판단

---

### **2. 문제 2: 동기화 메커니즘 부재**

```c
// ❌ 문제: 인터럽트 동기화 없음
volatile float motor_speed_rpm = 0.0f;      // volatile만 있고 lock이 없음
volatile uint32_t voltage_ref = 0;          // volatile만 있고 lock이 없음
volatile float calculated_rpm = 0.0f;       // volatile만 있고 lock이 없음
```

**문제점:**
- `volatile` 키워드만으로는 **32-bit 이상 데이터의 원자성(Atomicity) 보장 불가**
- Cortex-M7은 32-bit까지 원자적 접근이 가능하지만, float (32-bit)은 CPU 아키텍처에 따라 위험
- **실제 위험**: 중간에 인터럽트 발생 → 부분적으로 업데이트된 값 읽음

---

### **3. 문제 3: 인터럽트 우선순위 미정의**

```c
// ⚠️ NVIC 설정에서 우선순위가 명확하지 않음
NVIC_EnableIRQ(EXTI0_IRQn);  // 기본값으로 설정됨
NVIC_EnableIRQ(EXTI1_IRQn);
NVIC_EnableIRQ(EXTI2_IRQn);

// TIM1 인터럽트도 기본값
```

**문제점:**
- EXTI와 TIM1 중 어느 것이 우선인지 불명확
- 우선순위 역전(Priority Inversion) 가능성
- 예: TIM1 인터럽트가 진행 중 → EXTI 발생 → 문제 발생

---

### **4. 문제 4: ADC 읽기 시 대기 (Blocking)**

```c
// ⚠️ TIM1 인터럽트 핸들러에서 ADC 완료 대기
void Motor_ReadADC(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC1->SR & ADC_SR_EOC));  // ◄◄◄ 여기서 대기! (수 µs)
    result = ADC1->DR;
    
    ADC2->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC2->SR & ADC_SR_EOC));  // ◄◄◄ 여기서도 대기!
    
    ADC3->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC3->SR & ADC_SR_EOC));  // ◄◄◄ 여기서도 대기!
}
```

**문제점:**
- TIM1 인터럽트 핸들러 내에서 **polling 대기** → 인터럽트 레이턴시 증가
- 이 시간 동안 **우선순위 높은 다른 인터럽트 차단** 불가
- 하지만 EXTI가 TIM1보다 높으면 괜찮음 (우선순위에 따라)

---

## ✅ 현재 상황 요약

| 항목 | 상태 | 심각도 |
|-----|------|--------|
| **1ms 작업과 인터럽트 분리** | ❌ **미분리** | 🔴 |
| **공유 변수 보호** | ❌ **없음** | 🔴 |
| **데이터 경쟁 (Data Race)** | ✓ **발생 가능** | 🔴 |
| **인터럽트 우선순위 정의** | ❌ **미정의** | 🟡 |
| **동기화 메커니즘** | ❌ **없음** | 🔴 |
| **전체 아키텍처** | ⚠️ **취약** | 🔴 |

---

## 💡 권장 개선 방안

### **방안 1: 인터럽트 비활성화를 통한 임계 영역 보호 (권장)**

```c
// motor_control.c - Motor_ProcessSpeed()
static inline void Motor_ProcessSpeed(void)
{
    uint32_t primask = __get_PRIMASK();  // 현재 인터럽트 상태 저장
    __disable_irq();                     // 모든 인터럽트 비활성화
    
    float rpm_snapshot = calculated_rpm; // 공유 변수 읽기
    
    __set_PRIMASK(primask);              // 인터럽트 복원
    
    LPF(rpm_snapshot, Ft, &motor_speed_rpm);
}
```

**장점**: 간단, 신뢰성 높음
**단점**: 인터럽트 지연 가능 (하지만 임계 영역이 매우 작으므로 무시 가능)

---

### **방안 2: Semaphore/Mutex를 통한 동기화**

```c
// 헤더 파일에 추가
volatile uint8_t rpm_lock = 0;

// Hall.c - EXTI 핸들러
void SpeedCal(void)
{
    rpm_lock = 1;  // 진입 알림
    // ... RPM 계산 ...
    calculated_rpm = new_value;
    rpm_lock = 0;  // 완료
}

// motor_control.c - 1ms 인터럽트
void Motor_ProcessSpeed(void)
{
    while(rpm_lock);  // rpm_lock이 0이 될 때까지 대기
    float rpm_snapshot = calculated_rpm;
    LPF(rpm_snapshot, Ft, &motor_speed_rpm);
}
```

**장점**: 더 세밀한 제어
**단점**: 구현 복잡, deadlock 위험

---

### **방안 3: 더블 버퍼링 (Double Buffering)**

```c
volatile float calculated_rpm_buffer[2];  // 2개 버퍼
volatile uint8_t rpm_write_idx = 0;       // 쓰기 인덱스
volatile uint8_t rpm_read_idx = 1;        // 읽기 인덱스

// EXTI 핸들러 (Hall.c)
void SpeedCal(void)
{
    // ...
    calculated_rpm_buffer[rpm_write_idx] = new_rpm;
    rpm_write_idx = (rpm_write_idx + 1) & 1;  // 0과 1 사이 전환
}

// 1ms 인터럽트 (motor_control.c)
void Motor_ProcessSpeed(void)
{
    float rpm_snapshot = calculated_rpm_buffer[rpm_read_idx];
    rpm_read_idx = (rpm_read_idx + 1) & 1;
    LPF(rpm_snapshot, Ft, &motor_speed_rpm);
}
```

**장점**: 대기 없음, 원자적 연산 불필요
**단점**: 메모리 증가, 약간의 지연

---

### **방안 4: 인터럽트 우선순위 명시**

```c
// Hall.c - Initialize_Hall_Sensors()에 추가
NVIC_SetPriority(EXTI0_IRQn, 1);  // EXTI 높은 우선순위
NVIC_SetPriority(EXTI1_IRQn, 1);
NVIC_SetPriority(EXTI2_IRQn, 1);

// stm32f7xx_it.c 또는 main.c
NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 2);  // TIM1 낮은 우선순위
NVIC_SetPriority(SysTick_IRQn, 3);        // SysTick 가장 낮은 우선순위
```

---

## 🎯 즉시 조치 사항

### **1. 우선순위 설정 (즉시)**
```c
// Hall.c의 Initialize_Hall_Sensors() 함수 끝에 추가
NVIC_SetPriority(EXTI0_IRQn, 1);
NVIC_SetPriority(EXTI1_IRQn, 1);
NVIC_SetPriority(EXTI2_IRQn, 1);
NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 2);  // main.c 또는 timer.c에서
```

### **2. 임계 영역 보호 (우선순위)**
```c
// motor_control.c의 Motor_ProcessSpeed() 수정
static inline void Motor_ProcessSpeed(void)
{
    __disable_irq();                    // 보호 시작
    float rpm_snapshot = calculated_rpm;
    __enable_irq();                     // 보호 종료
    
    LPF(rpm_snapshot, Ft, &motor_speed_rpm);
}

// motor_control.c의 Motor_CheckRpmStuck() 수정
static inline void Motor_CheckRpmStuck(void)
{
    __disable_irq();
    RpmNew = calculated_rpm;
    __enable_irq();
    
    // ... 나머지 코드 ...
}
```

### **3. HallSum 보호**
```c
// motor_control.c의 Motor_UpdateInverterOutput() 수정
static inline void Motor_UpdateInverterOutput(void)
{
    if(FltFlg ==0 && InitCal ==1)
    {
        __disable_irq();
        uint8_t hall_snapshot = HallSum;
        __enable_irq();
        
        Update_Switching_Pattern(hall_snapshot);
    }
    else
    {
        // ...
    }
}
```

---

## 📝 결론

**현재 시스템은 1ms 작업(Task_1ms)과 인터럽트가 제대로 분리되지 않아 있습니다.**

주요 문제점:
1. ✗ 공유 변수(`calculated_rpm`, `HallSum`, `ias/ibs/ics`)에 대한 보호 메커니즘 부재
2. ✗ 데이터 경쟁 가능성 높음
3. ✗ 인터럽트 우선순위 미정의
4. ✗ 동기화 메커니즘 전무

**권장 조치:**
1. ✅ 인터럽트 우선순위 명시적 설정
2. ✅ 공유 변수 접근 시 `__disable_irq()` / `__enable_irq()` 임계 영역 설정
3. ✅ 가능하면 double buffering 검토
4. ✅ 테스트: 고속 회전 상태에서 모터 제어 안정성 검증

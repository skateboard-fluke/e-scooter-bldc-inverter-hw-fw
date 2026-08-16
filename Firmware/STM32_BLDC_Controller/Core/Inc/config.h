/*
 * config.h
 *
 *  Created on: Aug 10, 2026
 *      Author: kdk78
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_


// ---------- Math ----------
#ifndef PI
#define PI 3.14159f
#endif

// Hall 엣지 수 (한 바퀴당 인터럽트 수
//소형 BLDC모터:24.0f, 인휠모터:90.0f로 수정
#ifndef HALL_EDGES_PER_REV
#define HALL_EDGES_PER_REV 24.0f
#endif


// 1: 시계방향(CW), 0: 반시계(CCW)
#ifndef MOTOR_DIR_CW
#define MOTOR_DIR_CW 1
#define MOTOR_DIR_CCW 0
#endif

#ifndef MOTOR_DIR
#define MOTOR_DIR MOTOR_DIR_CW
#endif

// ---------- ADC & Sensors ----------
#define ADC_VREF 3.3f
#define ADC_FS 4095.0f
#define VDIV_RATIO 0.057362f // Vdc 측정 저항분배 비율
#define OFFSET_Volt 1.65f
#define OPAMP_GAIN 0.044f
// ---------- Thresholds ----------
#define THROTTLE_OFF_V 0.10f
#define THROTTLE_ON_V 0.20f
#define THROTTLE_MAX_V 1.55f

#define DUTY_MIN_TARGET 1000.0f
#define DUTY_MAX_TARGET 4000.0f

#define OC_LEVEL 35.0f
#define OC_TRIP_COUNT 50 // 2.5ms@20kHz 예시

#define MIN_RPM_TARGET   0.0f      // 쓰로틀 OFF
#define MIN_RUN_RPM      150.0f    // 기동 최소 속도
#define MAX_RPM_TARGET   4000.0f   // 정격 최고 속도 (4000 RPM)


#endif /* INC_CONFIG_H_ */

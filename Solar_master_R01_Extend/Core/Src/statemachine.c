



/*
 * statemachine.c
 *
 *  Created on: Mar 1, 2026
 *      Author: appletea
 *
 *  [Main role]
 *  ---------------------------------------------------------
 *  리모컨 UART1 수신과 주행 분배만 담당
 *
 *  역할
 *    - UART1 1바이트 수신
 *    - app_bms에 토글 명령 전달
 *    - manual 모드면 safe_drive_manual 호출
 *    - auto 모드면 safe_drive_auto 호출
 */

#include "statemachine.h"

#include "app_bms.h"
#include "app_proto.h"
#include "safe_drive.h"
#include "safe_drive_manual.h"
#include "tim.h"
#include "usart.h"
#include "speed.h"
#include "direction.h"
#include "car.h"

extern UART_HandleTypeDef huart1;

/* =========================================================
 * UART1 수신 상태
 * ========================================================= */
static volatile uint8_t rx_data[1];
static volatile uint8_t rx_flag = 0U;
static uint8_t rx_cmd = 0U;


/* =========================================================
 * 초기화
 * ========================================================= */
void STMACHINE_Init(void)
{
    /* UART1 1바이트 수신 시작 */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)rx_data, 1);

    /* 초음파 초기화 */
    Ultrasonic_Init();

    /* speed 모듈에 TIM2 채널 연결 */
    Speed_Init(&htim2, TIM_CHANNEL_1, &htim2, TIM_CHANNEL_2);

    /* PWM 시작 + 초기 STOP */
    Car_Init();

    /* auto 모드 상태 초기화 */
    SafeDriveAuto_Init();
}

/* =========================================================
 * UART1 수신 콜백
 * ---------------------------------------------------------
 * 리모컨 문자 수신 시
 * 1) 내부 버퍼 저장
 * 2) app_bms에 알림
 * 3) 다음 수신 재시작
 * ========================================================= */
void STMACHINE_UartRxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return;
    }

    if (huart->Instance == USART1)
    {
        rx_cmd = rx_data[0];
        rx_flag = 1U;

        /* app_bms에 토글 명령 전달 */
        App_Bms_NotifyRemoteCmd(rx_cmd);

        /* 다음 수신 재시작 */
        HAL_UART_Receive_IT(&huart1, (uint8_t *)rx_data, 1);
    }
}


/* =========================================================
 * 외부 명령 주입
 * ---------------------------------------------------------
 * UART2(moserial) 등 UART1 이외 경로에서 명령을 넣을 때 사용
 * ========================================================= */
void STMACHINE_SubmitCmd(uint8_t cmd)
{
    rx_cmd = cmd;
    rx_flag = 1U;
    App_Bms_NotifyRemoteCmd(cmd);
}

/* =========================================================
 * 기존 호환용
 * ---------------------------------------------------------
 * A로 auto 진입 시 auto 상태를 scan부터 시작하도록 맞춘다.
 * manual 복귀 시 즉시 정지시켜 잔류 명령을 막는다.
 * ========================================================= */
void ST_FLAG(uint8_t cmd)
{
    if (cmd == 'A')
    {
        if (App_Bms_IsAutoMode() != 0U)
        {
            SafeDriveAuto_Init();
        }
        else
        {
            SafeDrive_Stop();
        }
    }
}

/* =========================================================
 * 상위 task
 * ---------------------------------------------------------
 * 1) UART1 새 문자 처리
 * 2) manual 모드면 manual safe drive 처리
 * 3) auto 모드면 auto safe drive 처리
 * ========================================================= */
void ST_MACHINE(void)
{
    if (rx_flag != 0U)		// UART1 수신 플래그
    {
        /* 필요 시 echo */
        HAL_UART_Transmit(&huart1, (uint8_t *)&rx_cmd, 1, 10);

        ST_FLAG(rx_cmd);							// Manual / Auto 판단

        if (App_Bms_IsManualMode() != 0U)
        {
            SafeDriveManual_HandleCmd(rx_cmd);		// 수동
        }

        rx_flag = 0U;
    }

    if (App_Bms_IsAutoMode() != 0U)
    {
        SafeDriveAuto_Task();		// 자율 주행
    }
}








//========= 동원이형 코드=================================================//
///*
// * statemachine.c
// *
// *  Created on: Mar 1, 2026
// *      Author: appletea
// */
//
//#include "statemachine.h"
//
//
//
//
//extern UART_HandleTypeDef huart1;
//static volatile uint8_t rxData[1];
//static volatile uint8_t rxFlag = 0;
//static uint8_t rxCmd = 0;
//static AUTO_STATE auto_st = AUTO_STATE_STOP;
//static uint32_t auto_tick = 0;
//
//
//void STMACHINE_Init(void)
//{
//    // UART1 수신 인터럽트 시작
//    HAL_UART_Receive_IT(&huart1, (uint8_t *)rxData, 1);
//
//    // 초음파 함수 초기화
//    Ultrasonic_Init();
//
//    // 1) speed 모듈에 TIM2 채널 연결
//    Speed_Init(&htim2, TIM_CHANNEL_1, &htim2, TIM_CHANNEL_2);
//
//    // 2) PWM Start + 초기 STOP
//    Car_Init();
//}
//
//// UART 수신 콜백 함수를 여기로 이동
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART1)
//    {
//        rxCmd = rxData[0];
//        rxFlag = 1;
//        // 다음 수신 대기
//        HAL_UART_Receive_IT(&huart1, (uint8_t *)rxData, 1);
//    }
//}
//
//
//// 디버깅 화면에서 관찰할 변수
//uint8_t debug_current_spd = 0;
//
//
/////*** MANUAL 모드 로직 ***///
//static void DC_CONTROL_MANUAL(uint8_t cmd)
//{
//    switch(cmd)
//    {
//        case 'F':
//        	debug_current_spd = 100;
//        	Car_Move(CAR_FRONT, SPD_100);
//            break;
//        case 'Q':
//        	debug_current_spd = 50;
//			Car_Move(CAR_FRONT, SPD_50);
//			break;
//        case 'B':
//        	debug_current_spd = 100;
//        	Car_Move(CAR_BACK, SPD_100);
//            break;
//        case 'W':
//        	debug_current_spd = 50;
//			Car_Move(CAR_BACK, SPD_50);
//			break;
//        case 'L':
//        	debug_current_spd = 100;
//        	Car_Move(CAR_LEFT, SPD_100);
//            break;
//        case 'E':
//        	debug_current_spd = 50;
//			Car_Move(CAR_LEFT, SPD_50);
//			break;
//        case 'R':
//        	debug_current_spd = 100;
//        	Car_Move(CAR_RIGHT, SPD_100);
//            break;
//        case 'T':
//        	debug_current_spd = 50;
//			Car_Move(CAR_RIGHT, SPD_50);
//			break;
//        case 'S':
//        	Car_Stop();
//        	break;
//        default:
//            break;
//    }
//}
//
/////*** UART2(테스트용) ***///
//
//const static uint32_t waitTick = 200;
//static uint32_t prevTick = 0;
//
//void SHOW_UART2()
//{
//	Ultrasonic_TriggerAll();
//	uint32_t currentTick = HAL_GetTick();
//		if ((currentTick - prevTick) < waitTick) return; // 200ms 아직 안 됨
//		prevTick = currentTick;
//	printf("LEFT : %d cm\r\n CENTER : %d cm\r\n RIGHT : %d cm\r\n",
//			Ultrasonic_GetDistanceCm(US_LEFT),
//			Ultrasonic_GetDistanceCm(US_CENTER),
//			Ultrasonic_GetDistanceCm(US_RIGHT));
//}
//
//
//
//
//
//
//void DC_CONTROL_AUTO() {
//    Ultrasonic_TriggerAll();
//    uint32_t current_Tick = HAL_GetTick();
//
//    uint8_t L = Ultrasonic_GetDistanceCm(US_LEFT);
//    uint8_t C = Ultrasonic_GetDistanceCm(US_CENTER);
//    uint8_t R = Ultrasonic_GetDistanceCm(US_RIGHT);
//
//    // 100 이상의 거리는 100으로 통일
//    uint8_t DisLeft   = (L == 0 || L > 100) ? 100 : L;
//    uint8_t DisCenter = (C == 0 || C > 100) ? 100 : C;
//    uint8_t DisRight  = (R == 0 || R > 100) ? 100 : R;
//
//    //3개 반사값 비교해서 우선판단권 넘김
//    uint8_t front_min = DisCenter;
//    if (DisLeft < front_min)  front_min = DisLeft;
//    if (DisRight < front_min) front_min = DisRight;
//
//    switch (auto_st) {
//        case AUTO_STATE_SCAN:
//            // 최우선 순위 정면 충돌 직전 시 500ms 후진
//            if (DisCenter < Crash_Distance) {
//                Car_Move(CAR_BACK, SPD_50);
//                auto_tick = current_Tick;
//                auto_st = AUTO_STATE_BACK;
//            }
//            // 본격적인 회피
//            else if (front_min < Block_Distance_Front) {
//                // 이중 비교 및 양 옆센서 값이 너무 낮을때 회피
//            	if (DisLeft < (Block_Distance_Side)) {
//                    Car_Move(CAR_RIGHT, SPD_80);
//                }
//                else if (DisRight < (Block_Distance_Side)) {
//                    Car_Move(CAR_LEFT, SPD_80);
//                }
//            	// 추가적인 우회전 좌회전판단
//                else {
//                    int diff = (int)DisLeft - (int)DisRight;
//                    if (diff > 6)       Car_Move(CAR_LEFT, SPD_80);
//                    else if (diff < -6) Car_Move(CAR_RIGHT, SPD_80);
//                    else                 Car_Move(CAR_LEFT, SPD_80);
//                }
//                auto_tick = current_Tick;
//                auto_st = AUTO_STATE_AVOID;
//            }
//            // 일반 주행 및 벽 거리 멀어지게
//            else {
//                // 벽에 너무 가까우면 회피
//                if (DisLeft < 15)       Car_Move(CAR_RIGHT, SPD_65);
//                else if (DisRight < 15) Car_Move(CAR_LEFT, SPD_65);
//
//                // 2. [사용자님 아이디어] 한쪽이 60 이상 넓게 뚫려 있다면 미리 몸을 틀어 코너 대비
//                else if (DisLeft >= 70 && DisRight < 70) Car_Move(CAR_LEFT, SPD_65);
//                else if (DisRight >= 70 && DisLeft < 70) Car_Move(CAR_RIGHT, SPD_65);
//
//                // 3. 양쪽 다 좁거나, 양쪽 다 60 이상으로 뻥 뚫려 있으면 직진
//                else                    Car_Move(CAR_FRONT, SPD_65);
//            }
//            break;
//
//        case AUTO_STATE_AVOID:
//
//            if (front_min > (Block_Distance_Front) || (current_Tick - auto_tick >= 40)) {
//                auto_st = AUTO_STATE_SCAN;
//            }
//            break;
//
//        case AUTO_STATE_BACK:
//            if (current_Tick - auto_tick >= 250) {
//                Car_Stop();
//                auto_st = AUTO_STATE_SCAN;
//            }
//            break;
//
//        default:
//            auto_st = AUTO_STATE_SCAN;
//            break;
//    }
//}
//
//
//
//
//
//
//
//
//static bool st_auto = 0;
//static bool st_manual = 1;
//
////*** AUTO MANUAL 판단 ***//
//void ST_FLAG(uint8_t cmd)
//{
//	if(cmd == 'A')
//	{
//		st_auto = 1;
//		st_manual = 0;
//		auto_st = AUTO_STATE_SCAN;
//	}
//	if(cmd == 'P')
//	{
//		Car_Stop();
//		st_auto = 0;
//		st_manual = 1;
//		auto_st = AUTO_STATE_SCAN;
//
//	}
//}
//
//
//
//
//void ST_MACHINE() {
//	// BlueTooth UART1 수신
//	if (rxFlag)
//	{
//		HAL_UART_Transmit(&huart1, (uint8_t*) &rxCmd, 1, 10);
//		ST_FLAG(rxCmd);
//
//		if (st_manual == 1)
//		{
//			DC_CONTROL_MANUAL(rxCmd);
//		}
//		rxFlag = 0;
//	}
//	if (st_auto == 1)
//	{
//		DC_CONTROL_AUTO();
//	}
//
//}






/*
 * safe_drive_auto.c
 *
 *  Created on: 2026. 3. 23.
 *
 *  [Main role]
 *  ---------------------------------------------------------
 *  초음파 기반 auto 주행 로직
 *
 *  핵심 동작
 *  ---------------------------------------------------------
 *  1) 초음파 3개 거리 측정
 *  2) 정면 충돌 위험이면 후진
 *  3) 장애물 접근 시 좌/우 회피
 *  4) 일반 구간에서는 직진 또는 벽과 거리 보정
 *
 *  주의
 *  ---------------------------------------------------------
 *  - 실제 주행 출력은 SafeDrive_Move() / SafeDrive_Stop()로 수행한다.
 *  - 따라서 safety 감속 / 정지 정책이 자동주행에도 동일하게 반영된다.
 */

#include "safe_drive_auto.h"

#include "safe_drive.h"
#include "ultrasonic.h"
#include "direction.h"
#include "speed.h"

/* =========================================================
 * auto 주행 기준값
 * ========================================================= */
#define BLOCK_DISTANCE_FRONT      35U
#define BLOCK_DISTANCE_SIDE       20U
#define CRASH_DISTANCE            10U

/* =========================================================
 * 내부 상태
 * ========================================================= */
static AUTO_STATE s_auto_st = AUTO_STATE_STOP;
static uint32_t s_auto_tick = 0U;

/* =========================================================
 * 초기화
 * ========================================================= */
void SafeDriveAuto_Init(void)
{
    s_auto_st = AUTO_STATE_SCAN;
    s_auto_tick = HAL_GetTick();
}

/* =========================================================
 * auto task
 * ========================================================= */
void SafeDriveAuto_Task(void)
{
    uint32_t now;
    uint8_t l_raw;
    uint8_t c_raw;
    uint8_t r_raw;
    uint8_t l;
    uint8_t c;
    uint8_t r;
    uint8_t front_min;

    Ultrasonic_TriggerAll();
    now = HAL_GetTick();

    l_raw = Ultrasonic_GetDistanceCm(US_LEFT);
    c_raw = Ultrasonic_GetDistanceCm(US_CENTER);
    r_raw = Ultrasonic_GetDistanceCm(US_RIGHT);

    /* 0 또는 100 초과 값은 100으로 정규화 */
    l = (l_raw == 0U || l_raw > 100U) ? 100U : l_raw;
    c = (c_raw == 0U || c_raw > 100U) ? 100U : c_raw;
    r = (r_raw == 0U || r_raw > 100U) ? 100U : r_raw;

    front_min = c;
    if (l < front_min) front_min = l;
    if (r < front_min) front_min = r;

    switch (s_auto_st)
    {
        case AUTO_STATE_SCAN:
            /* 정면 충돌 직전이면 짧게 후진 */
            if (c < CRASH_DISTANCE)
            {
                SafeDrive_Move(CAR_BACK, SPD_50);
                s_auto_tick = now;
                s_auto_st = AUTO_STATE_BACK;
            }
            /* 회피 구간 */
            else if (front_min < BLOCK_DISTANCE_FRONT)
            {
                if (l < BLOCK_DISTANCE_SIDE)
                {
                    SafeDrive_Move(CAR_RIGHT, SPD_80);
                }
                else if (r < BLOCK_DISTANCE_SIDE)
                {
                    SafeDrive_Move(CAR_LEFT, SPD_80);
                }
                else
                {
                    int diff = (int)l - (int)r;

                    if (diff > 6)
                    {
                        SafeDrive_Move(CAR_LEFT, SPD_80);
                    }
                    else if (diff < -6)
                    {
                        SafeDrive_Move(CAR_RIGHT, SPD_80);
                    }
                    else
                    {
                        SafeDrive_Move(CAR_LEFT, SPD_80);
                    }
                }

                s_auto_tick = now;
                s_auto_st = AUTO_STATE_AVOID;
            }
            /* 일반 주행 */
            else
            {
                if (l < 15U)
                {
                    SafeDrive_Move(CAR_RIGHT, SPD_65);
                }
                else if (r < 15U)
                {
                    SafeDrive_Move(CAR_LEFT, SPD_65);
                }
                else if ((l >= 70U) && (r < 70U))
                {
                    SafeDrive_Move(CAR_LEFT, SPD_65);
                }
                else if ((r >= 70U) && (l < 70U))
                {
                    SafeDrive_Move(CAR_RIGHT, SPD_65);
                }
                else
                {
                    SafeDrive_Move(CAR_FRONT, SPD_65);
                }
            }
            break;

        case AUTO_STATE_AVOID:
            if ((front_min > BLOCK_DISTANCE_FRONT) ||
                ((now - s_auto_tick) >= 40U))
//                ((now - s_auto_tick) >= 350U))
            {
                s_auto_st = AUTO_STATE_SCAN;
            }
            break;

        case AUTO_STATE_BACK:
            if ((now - s_auto_tick) >= 250U)
            {
                SafeDrive_Stop();
                s_auto_st = AUTO_STATE_SCAN;
            }
            break;

        default:
            s_auto_st = AUTO_STATE_SCAN;
            break;
    }
}

/* =========================================================
 * getter
 * ========================================================= */
AUTO_STATE SafeDriveAuto_GetState(void)
{
    return s_auto_st;
}

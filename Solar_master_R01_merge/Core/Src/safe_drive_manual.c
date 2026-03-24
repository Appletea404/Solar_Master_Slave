/*
 * safe_drive_manual.c
 *
 *  Created on: 2026. 3. 23.
 *
 *  [Main role]
 *  ---------------------------------------------------------
 *  수동 주행 명령 -> SafeDrive 출력 연결
 *
 *  처리 문자
 *  ---------------------------------------------------------
 *    F : 전진 100%
 *    Q : 전진 50%
 *    B : 후진 100%
 *    W : 후진 50%
 *    L : 좌회전 100%
 *    E : 좌회전 50%
 *    R : 우회전 100%
 *    T : 우회전 50%
 *    S : 정지
 *
 *  주의
 *  ---------------------------------------------------------
 *  - A / P / D / K 는 app_bms가 처리한다.
 *  - 여기서는 순수하게 "주행 문자"만 처리한다.
 */

#include "safe_drive_manual.h"

#include "safe_drive.h"
#include "direction.h"
#include "speed.h"

/* =========================================================
 * 내부 디버그 값
 * ========================================================= */
static uint8_t s_last_cmd = 0U;
static uint8_t s_last_req_pct = 0U;

/* =========================================================
 * 수동 명령 처리
 * ========================================================= */
uint8_t SafeDriveManual_HandleCmd(uint8_t cmd)
{
    s_last_cmd = cmd;

    switch (cmd)
    {
        case 'F':
            s_last_req_pct = 100U;
            SafeDrive_Move(CAR_FRONT, SPD_100);
            return 1U;

        case 'Q':
            s_last_req_pct = 50U;
            SafeDrive_Move(CAR_FRONT, SPD_50);
            return 1U;

        case 'B':
            s_last_req_pct = 100U;
            SafeDrive_Move(CAR_BACK, SPD_100);
            return 1U;

        case 'W':
            s_last_req_pct = 50U;
            SafeDrive_Move(CAR_BACK, SPD_50);
            return 1U;

        case 'L':
            s_last_req_pct = 100U;
            SafeDrive_Move(CAR_LEFT, SPD_100);
            return 1U;

        case 'E':
            s_last_req_pct = 50U;
            SafeDrive_Move(CAR_LEFT, SPD_50);
            return 1U;

        case 'R':
            s_last_req_pct = 100U;
            SafeDrive_Move(CAR_RIGHT, SPD_100);
            return 1U;

        case 'T':
            s_last_req_pct = 50U;
            SafeDrive_Move(CAR_RIGHT, SPD_50);
            return 1U;

        case 'S':
            s_last_req_pct = 0U;
            SafeDrive_Stop();
            return 1U;

        default:
            /* 주행 문자가 아니면 처리하지 않음 */
            return 0U;
    }
}

/* =========================================================
 * getter
 * ========================================================= */
uint8_t SafeDriveManual_GetLastCmd(void)
{
    return s_last_cmd;
}

uint8_t SafeDriveManual_GetLastReqPct(void)
{
    return s_last_req_pct;
}








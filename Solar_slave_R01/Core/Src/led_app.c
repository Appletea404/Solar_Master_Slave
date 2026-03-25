/*
 * led_app.c
 *
 *  Created on: Mar 22, 2026
 *      Author: parkdoyoung
 */




//#include <stdio.h>
//
///* =========================================================
// * LED 핀 기본값
// * ---------------------------------------------------------
// * 실제 보드 LED 핀에 맞게 확인 필요
// * 현재는 PC8 기준
// * ========================================================= */
//#ifndef APP_LED_GPIO_Port
//#define APP_LED_GPIO_Port      GPIOC
//#endif
//
//#ifndef APP_LED_Pin
//#define APP_LED_Pin            GPIO_PIN_8
//#endif
//
//#ifndef APP_LED_ACTIVE_HIGH
//#define APP_LED_ACTIVE_HIGH    1U
//#endif

/* =========================================================
 * 내부 helper
 * ========================================================= */
//static void LedApp_Set(uint8_t on)
//{
//#if (APP_LED_ACTIVE_HIGH == 1U)
//    HAL_GPIO_WritePin(APP_LED_GPIO_Port,
//                      APP_LED_Pin,
//                      (on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
//#else
//    HAL_GPIO_WritePin(APP_LED_GPIO_Port,
//                      APP_LED_Pin,
//                      (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
//#endif
//}

/* =========================================================
 * 공개 함수
 * ========================================================= */
//void LedApp_Init(void)
//{
//    LedApp_Set(0U);
//}



//
//void LedApp_Task(void)
//{
//    CanFrame_t rx;
//
//    /* 1. CAN 버스로부터 메시지 수신 확인 */
//    if (Can_ReadFrame(&rx) == 0U)
//    {
//        return;
//    }
//
//    /* 2. 특정 ID(APP_CAN_ID_CTRL)로부터 온 메시지인지 확인 */
//    if (rx.id == APP_CAN_ID_CTRL)
//    {
//        /* 데이터가 최소 1바이트 이상 존재하는지 확인 */
//        if (rx.dlc >= 1U)
//        {
//            /* 첫 번째 데이터 바이트(data[0])에 담긴 문자를 판별 */
//            switch (rx.data[0])
//            {
//                case 'D' :  /* Drive 모드 문자 수신 시 */
//                    printf("[MSG] 'D' Received - Executing Action 1\r\n");
//
//                    /* =============================================
//                     * 여기에 [특정동작1] 코드를 작성하세요.
//                     * 예 : DC_Motor_Forward(); 또는 Status = DRIVE;
//                     * ============================================= */
//                    break;
//
//                case 'P' :  /* Parking 모드 문자 수신 시 */
//                    printf("[MSG] 'P' Received - Executing Action 2\r\n");
//
//                    /* =============================================
//                     * 여기에 [특정동작2] 코드를 작성하세요.
//                     * 예 : DC_Motor_Stop(); 또는 Status = PARKING;
//                     * ============================================= */
//                    break;
//
//                default :
//                    /* D나 P 외에 다른 문자가 들어왔을 때의 처리 (필요 시) */
//                    break;
//            }
//        }
//    }
//}

//void LedApp_Task(void)
//{
//    CanFrame_t rx;
//    uint8_t i;
//
//    if (Can_ReadFrame(&rx) == 0U)
//    {
//        return;
//    }
//
//    printf("[APP][RX] id=0x%03X dlc=%u", rx.id, rx.dlc);
//
//    for (i = 0U; i < rx.dlc; i++)
//    {
//        printf(" %02X", rx.data[i]);
//    }
//
//    printf("\r\n");
//
//    if ((rx.id == APP_CAN_ID_CTRL) && (rx.dlc >= 2U))
//    {
//        if (rx.data[0] == APP_CMD_LED)
//        {
//            if (rx.data[1] == APP_LED_ON)
//            {
//                LedApp_Set(1U);
//                printf("[LED] ON\r\n");
//            }
//            else if (rx.data[1] == APP_LED_OFF)
//            {
//                LedApp_Set(0U);
//                printf("[LED] OFF\r\n");
//            }
//        }
//    }
//}

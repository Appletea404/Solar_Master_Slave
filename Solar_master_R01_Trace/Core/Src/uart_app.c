/*
 * uart_app.c
 *
 *  Created on: Mar 22, 2026
 *      Author: parkdoyoung
 */


/* uart_app.c
 *
 * NOTE: 이 모듈은 구버전 LED CAN 제어 코드입니다.
 *       app_proto.h에서 APP_CMD_LED 등이 제거되어 현재 비활성화 상태입니다.
 *       UART2 moserial 기능이 필요하면 uart_cmd.c 로 이식하세요.
 */
#if 0

#include "uart_app.h"
#include "usart.h"
#include "can.h"
#include "app_proto.h"

#include <stdio.h>
#include <string.h>

/* =========================================================
 * UART 명령 입력 앱
 * ---------------------------------------------------------
 * 터미널에서
 *   '1' -> LED ON 명령 전송
 *   '0' -> LED OFF 명령 전송
 *
 * 인터럽트에서는 받은 문자만 ring buffer에 넣고,
 * 실제 명령 해석과 CAN 송신 요청은 Task에서 수행한다.
 * ========================================================= */
#define UART_APP_RX_BUF_SIZE      16U

typedef struct
{
    uint8_t rx_byte;

    uint8_t ring[UART_APP_RX_BUF_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;

    uint8_t cmd_pending;
    uint8_t cmd_value;
} UartApp_t;

static UartApp_t g_uart_app;

/* =========================================================
 * 내부 helper
 * ========================================================= */
static void UartApp_StartRxIt(void)
{
    HAL_UART_Receive_IT(&huart2, &g_uart_app.rx_byte, 1U);
}

static void UartApp_PushChar(uint8_t ch)
{
    uint8_t next;

    next = (uint8_t)((g_uart_app.head + 1U) % UART_APP_RX_BUF_SIZE);

    if (next == g_uart_app.tail)
    {
        return;
    }

    g_uart_app.ring[g_uart_app.head] = ch;
    g_uart_app.head = next;
}

static uint8_t UartApp_PopChar(uint8_t *ch)
{
    if ((ch == NULL) || (g_uart_app.head == g_uart_app.tail))
    {
        return 0U;
    }

    *ch = g_uart_app.ring[g_uart_app.tail];
    g_uart_app.tail = (uint8_t)((g_uart_app.tail + 1U) % UART_APP_RX_BUF_SIZE);

    return 1U;
}

/* =========================================================
 * 공개 함수
 * ========================================================= */
void UartApp_Init(void)
{
    memset(&g_uart_app, 0, sizeof(g_uart_app));
    UartApp_StartRxIt();
}

void UartApp_Task(void)
{
    uint8_t ch;
    uint8_t data[2];

    /* 이전 명령이 아직 CAN 송신 대기중이면 먼저 처리 */
    if (g_uart_app.cmd_pending != 0U)
    {
        if (Can_IsReady() == 0U)
        {
            return;
        }

        data[0] = APP_CMD_LED;
        data[1] = g_uart_app.cmd_value;

        if (Can_SendStd(APP_CAN_ID_CTRL, 2U, data) != 0U)
        {
            printf("[APP][TX] id=0x%03X cmd=0x%02X val=0x%02X\r\n",
                   APP_CAN_ID_CTRL,
                   data[0],
                   data[1]);

            g_uart_app.cmd_pending = 0U;
        }

        return;
    }

    if (UartApp_PopChar(&ch) == 0U)
    {
        return;
    }

    if (ch == '1')
    {
        g_uart_app.cmd_pending = 1U;
        g_uart_app.cmd_value   = APP_LED_ON;
        printf("[UART] '1' -> LED ON request\r\n");
    }
    else if (ch == '0')
    {
        g_uart_app.cmd_pending = 1U;
        g_uart_app.cmd_value   = APP_LED_OFF;
        printf("[UART] '0' -> LED OFF request\r\n");
    }
    else if ((ch == '\r') || (ch == '\n'))
    {
        /* 엔터는 무시 */
    }
    else
    {
        printf("[UART] unsupported char: 0x%02X\r\n", ch);
    }
}

void UartApp_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2)
    {
        return;
    }

    UartApp_PushChar(g_uart_app.rx_byte);
    UartApp_StartRxIt();
}

#endif /* 0 */

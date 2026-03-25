/*
 * app_charger.c
 *
 *  Created on: 2026. 3. 20.
 *
 *  설명:
 *  ---------------------------------------------------------
 *  태양전지 충전 시스템 상위 app 구현
 *
 *  현재 버전의 핵심 포인트
 *  ---------------------------------------------------------
 *  1) 10ms 제어 / 500ms 로그 주기 분리
 *  2) UART2 DMA 로그 큐 기반 논블로킹 출력
 *  3) 현재 solar_sensing / charger_state 구조와 일치
 *  4) battery debug / sample age / debounce 상태 로그 포함
 */

#include "app_charger.h"

#include "i2c.h"
#include "tim.h"
#include "usart.h"

#include "solar_sensing.h"
#include "solar_pi_control.h"
#include "charger_state.h"
#include "dma.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* =========================================================
 * 내부 주기 설정
 * ========================================================= */
#define APP_CHARGER_CONTROL_PERIOD_MS          10U
#define APP_CHARGER_DEBUG_PERIOD_MS           500U

/* =========================================================
 * pending backlog 제한
 * ---------------------------------------------------------
 * 실시간 제어에서는 backlog를 무한정 쌓는 것보다
 * 일정 개수까지만 허용하는 편이 더 안전하다.
 * ========================================================= */
#define APP_CHARGER_MAX_PENDING_CTRL            5U
#define APP_CHARGER_MAX_PENDING_DBG             2U

/* =========================================================
 * UART2 DMA 로그 큐 설정
 * ---------------------------------------------------------
 * 초기화 로그 + 상태 로그 2~3줄을 고려해
 * 이전 버전보다 큐와 길이를 조금 더 여유 있게 잡는다.
 * ========================================================= */
#define APP_CHARGER_LOG_QUEUE_SIZE             16U
#define APP_CHARGER_LOG_MAX_LEN               384U

/* =========================================================
 * 내부 전역 객체
 * ========================================================= */
static SolarSensing_t   g_solar_sensor;
static SolarSensing_t   g_batt_sensor;
static SolarPiControl_t g_ctrl;
static ChargerState_t   g_charger;

/* 센서 초기화 결과 (SHOW_UART6_APP_CHARGER 에서 표시) */
static uint8_t g_solar_init_ok = 0U;
static uint8_t g_batt_init_ok  = 0U;

/* =========================================================
 * 내부 pending counter
 * ---------------------------------------------------------
 * 1ms tick ISR에서 증가시키고
 * main task에서 감소시키며 소비한다.
 * ========================================================= */
static volatile uint16_t g_pending_ctrl_10ms = 0U;
static volatile uint16_t g_pending_dbg_500ms = 0U;

/* =========================================================
 * UART2 DMA 로그 큐
 * ========================================================= */
static volatile uint8_t  g_uart2_tx_busy   = 0U;
static volatile uint8_t  g_log_queue_head  = 0U;
static volatile uint8_t  g_log_queue_tail  = 0U;
static volatile uint8_t  g_log_queue_count = 0U;
static volatile uint32_t g_log_drop_count  = 0U;

static char     g_log_queue[APP_CHARGER_LOG_QUEUE_SIZE][APP_CHARGER_LOG_MAX_LEN];
static uint16_t g_log_len[APP_CHARGER_LOG_QUEUE_SIZE];

/* =========================================================
 * 내부 함수 선언
 * ========================================================= */
static uint16_t AppCharger_Strnlen(const char *str, uint16_t max_len);
static void AppCharger_UartTryStartTx(void);
static void AppCharger_LogPrintf(const char *fmt, ...);

static void AppCharger_ApplyDuty(float duty);
static void AppCharger_PrintI2CError(const char *tag, I2C_HandleTypeDef *hi2c);
static void AppCharger_I2CScanBus(I2C_HandleTypeDef *hi2c, const char *bus_name);
static void AppCharger_PrintStatus(void);

/* =========================================================
 * 내부용 안전 길이 계산
 * ========================================================= */
static uint16_t AppCharger_Strnlen(const char *str, uint16_t max_len)
{
    uint16_t len = 0U;

    if (str == NULL)
    {
        return 0U;
    }

    while ((len < max_len) && (str[len] != '\0'))
    {
        len++;
    }

    return len;
}


/* =========================================================
 * [STATE] 로그용 시간 prefix 생성
 * ---------------------------------------------------------
 * HAL_GetTick() 기준 부팅 후 경과 시간을
 * [MM:SS.mmm] 형태 문자열로 만든다.
 *
 * 예)
 *   [00:12.345]
 *   [03:05.027]
 *
 * 주의:
 * - 분은 0~99까지만 2자리로 표시한다.
 * - 100분 이상 경과 시에도 00~99 범위로 순환 표시된다.
 *   필요하면 나중에 시:분:초 형태로 확장 가능하다.
 * ========================================================= */
static void AppCharger_GetTimePrefix(char *buf, uint16_t buf_len)
{
    uint32_t tick_ms;
    uint32_t total_sec;
    uint32_t min;
    uint32_t sec;
    uint32_t msec;

    if ((buf == NULL) || (buf_len == 0U))
    {
        return;
    }

    tick_ms = HAL_GetTick();

    total_sec = tick_ms / 1000U;
    min  = (total_sec / 60U) % 100U;
    sec  = total_sec % 60U;
    msec = tick_ms % 1000U;

    (void)snprintf(buf,
                   buf_len,
                   "[%02lu:%02lu.%03lu]",
                   (unsigned long)min,
                   (unsigned long)sec,
                   (unsigned long)msec);
}


/* =========================================================
 * UART 송신 시작
 * ---------------------------------------------------------
 * 큐에 데이터가 있고 UART2 DMA가 idle이면
 * tail 슬롯을 즉시 송신 시작한다.
 * ========================================================= */
static void AppCharger_UartTryStartTx(void)
{
    HAL_StatusTypeDef st;
    uint8_t slot_index;
    uint16_t tx_len;

    __disable_irq();

    if ((g_uart2_tx_busy != 0U) || (g_log_queue_count == 0U))
    {
        __enable_irq();
        return;
    }

    g_uart2_tx_busy = 1U;
    slot_index = g_log_queue_tail;
    tx_len = g_log_len[slot_index];

    __enable_irq();

    st = HAL_UART_Transmit_DMA(&huart2,
                               (uint8_t *)g_log_queue[slot_index],
                               tx_len);

    if (st != HAL_OK)
    {
        __disable_irq();
        g_uart2_tx_busy = 0U;
        __enable_irq();
    }
}

/* =========================================================
 * 내부 printf helper
 * ========================================================= */
static void AppCharger_LogPrintf(const char *fmt, ...)
{
    /* static: 384바이트를 스택 대신 BSS에 배치 — 스택 오버플로우 방지
     * AppCharger_LogPrintf는 ISR에서 호출되지 않으므로 재진입 문제 없음 */
    static char buf[APP_CHARGER_LOG_MAX_LEN];
    va_list ap;

    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    App_Charger_LogString(buf);
}

/* =========================================================
 * UART2 DMA 기반 논블로킹 로그 송신
 * ========================================================= */
void App_Charger_LogString(const char *str)
{
    uint16_t len;
    uint8_t slot_index;
    uint8_t need_kick = 0U;

    if (str == NULL)
    {
        return;
    }

    len = AppCharger_Strnlen(str, (uint16_t)(APP_CHARGER_LOG_MAX_LEN - 1U));
    if (len == 0U)
    {
        return;
    }

    __disable_irq();

    if (g_log_queue_count >= APP_CHARGER_LOG_QUEUE_SIZE)
    {
        g_log_drop_count++;
        __enable_irq();
        return;
    }

    slot_index = g_log_queue_head;

    memcpy((void *)g_log_queue[slot_index], str, len);
    g_log_queue[slot_index][len] = '\0';
    g_log_len[slot_index] = len;

    g_log_queue_head++;
    if (g_log_queue_head >= APP_CHARGER_LOG_QUEUE_SIZE)
    {
        g_log_queue_head = 0U;
    }

    g_log_queue_count++;

    if (g_uart2_tx_busy == 0U)
    {
        need_kick = 1U;
    }

    __enable_irq();

    if (need_kick != 0U)
    {
        AppCharger_UartTryStartTx();
    }
}

/* =========================================================
 * UART DMA 송신 완료 callback 라우터
 * ========================================================= */
void App_Charger_UartTxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2)
    {
        return;
    }

    __disable_irq();

    if (g_log_queue_count > 0U)
    {
        g_log_queue_tail++;
        if (g_log_queue_tail >= APP_CHARGER_LOG_QUEUE_SIZE)
        {
            g_log_queue_tail = 0U;
        }

        g_log_queue_count--;
    }

    g_uart2_tx_busy = 0U;

    __enable_irq();

    AppCharger_UartTryStartTx();
}

/* =========================================================
 * UART DMA 에러 callback 라우터
 * ---------------------------------------------------------
 * 현재 슬롯을 버리고 다음 로그부터 재시도한다.
 * ========================================================= */
void App_Charger_UartErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2)
    {
        return;
    }

    __disable_irq();

    if (g_log_queue_count > 0U)
    {
        g_log_queue_tail++;
        if (g_log_queue_tail >= APP_CHARGER_LOG_QUEUE_SIZE)
        {
            g_log_queue_tail = 0U;
        }

        g_log_queue_count--;
    }

    g_uart2_tx_busy = 0U;

    __enable_irq();

    AppCharger_UartTryStartTx();
}

/* =========================================================
 * PWM duty 적용
 * ========================================================= */
static void AppCharger_ApplyDuty(float duty)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty < CONTROL_DUTY_MIN)
    {
        duty = CONTROL_DUTY_MIN;
    }

    if (duty > CONTROL_DUTY_MAX)
    {
        duty = CONTROL_DUTY_MAX;
    }

    arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    ccr = (uint32_t)(duty * (float)arr);

    if (ccr > arr)
    {
        ccr = arr;
    }

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr);
}

/* =========================================================
 * I2C 에러 출력
 * ========================================================= */
static void AppCharger_PrintI2CError(const char *tag, I2C_HandleTypeDef *hi2c)
{
    uint32_t err;

    err = HAL_I2C_GetError(hi2c);

    AppCharger_LogPrintf("%s failed, HAL_I2C_GetError=0x%08lX\r\n",
                         tag,
                         (unsigned long)err);
}

/* =========================================================
 * I2C 버스 스캔
 * ========================================================= */
static void AppCharger_I2CScanBus(I2C_HandleTypeDef *hi2c, const char *bus_name)
{
    uint8_t addr;
    HAL_StatusTypeDef ret;
    uint8_t found = 0U;

    AppCharger_LogPrintf("Scanning %s...\r\n", bus_name);

    for (addr = 1U; addr < 128U; addr++)
    {
        ret = HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(addr << 1), 2U, 50U);
        if (ret == HAL_OK)
        {
            AppCharger_LogPrintf("%s found device at 0x%02X\r\n", bus_name, addr);
            found = 1U;
        }
        else if (ret == HAL_TIMEOUT)
        {
            AppCharger_LogPrintf("%s bus timeout at 0x%02X, abort scan\r\n", bus_name, addr);
            break;
        }
    }

    if (found == 0U)
    {
        AppCharger_LogPrintf("%s no device found\r\n", bus_name);
    }
}

/* =========================================================
 * 상태 로그 출력
 * ---------------------------------------------------------
 * 1) 메인 충전 상태
 * 2) battery filter / range / status 디버그
 * 3) sample age / debounce / active fault 디버그
 *
 * 주의:
 * - 현재 solar_sensing 구조에서는 cnvr / ovf를 제거했으므로
 *   더 이상 출력하지 않는다.
 * ========================================================= */
static void AppCharger_PrintStatus(void)
{
    SolarSensingDebug_t batt_dbg;
    char time_prefix[16];

    SolarSensing_GetDebug(&g_batt_sensor, &batt_dbg);

    /* 시간 문자열 생성 */
    AppCharger_GetTimePrefix(time_prefix, (uint16_t)sizeof(time_prefix));

    AppCharger_LogPrintf(
        "%s [STATE:%s][MODE:%s][FAULT:%s] "
        "Vpv=%.3fV Ipv=%.1fmA Ppv=%.3fW | "
        "Vbat_bus=%.3fV Vbat=%.3fV Ichg=%.1fmA Pchg=%.3fW | "
        "Eff=%.1f%% SOC=%.1f%% Duty=%.3f | "
        "Iref=%.3fA Icc=%.3fA Icv=%.3fA Imppt=%.3fA\r\n",
        time_prefix, ChargerState_StateString(g_charger.state),
        SolarPiControl_ModeString(g_charger.control_mode_last),
        ChargerState_PrimaryFaultString(g_charger.fault_flags),

        g_charger.solar_src_v_last,
        g_charger.solar_i_last * 1000.0f,
        g_charger.solar_p_last,

        g_charger.batt_bus_v_last,
        g_charger.batt_v_last,
        g_charger.batt_i_last * 1000.0f,
        g_charger.batt_p_last,

        g_charger.eff_last,
        g_charger.soc_last,
        g_charger.duty_last,

        g_charger.i_ref_last,
        g_charger.i_cc_last,
        g_charger.i_cv_last,
        g_charger.i_mppt_last);

    AppCharger_LogPrintf(
        "[BATTDBG] raw_bus=%.3fV raw_src=%.3fV filt_v=%.3fV out_v=%.3fV "
        "raw_i=%.1fmA filt_i=%.1fmA out_i=%.1fmA | "
        "init=%u inv=%u val=%u range=%u st=%lu err=0x%08lX drop=%lu\r\n",
        batt_dbg.raw_bus_v,
        batt_dbg.raw_source_v,
        batt_dbg.filt_v,
        batt_dbg.out_bus_v,

        batt_dbg.raw_current_ma,
        batt_dbg.filt_i,
        batt_dbg.out_current_ma,

        (unsigned int)batt_dbg.filter_initialized,
        (unsigned int)batt_dbg.invalid_count,
        (unsigned int)batt_dbg.valid_count,
        (unsigned int)batt_dbg.range_valid,
        (unsigned long)batt_dbg.last_status,
        (unsigned long)batt_dbg.last_i2c_error,
        (unsigned long)g_log_drop_count);

    AppCharger_LogPrintf(
        "[SNSDBG] s_age=%lums b_age=%lums | "
        "s_comm=%u b_comm=%u s_rng=%u b_rng=%u | "
        "s_cf=%u b_cf=%u s_rf=%u b_rf=%u | "
        "fault=0x%08lX\r\n",
        (unsigned long)g_charger.solar_sample_age_ms,
        (unsigned long)g_charger.batt_sample_age_ms,

        (unsigned int)g_charger.solar_comm_ok,
        (unsigned int)g_charger.batt_comm_ok,
        (unsigned int)g_charger.solar_range_ok,
        (unsigned int)g_charger.batt_range_ok,

        (unsigned int)g_charger.solar_comm_fault_active,
        (unsigned int)g_charger.batt_comm_fault_active,
        (unsigned int)g_charger.solar_range_fault_active,
        (unsigned int)g_charger.batt_range_fault_active,

        (unsigned long)g_charger.fault_flags);
}

/* =========================================================
 * App_Charger_HwInit
 * ---------------------------------------------------------
 * 보드 전원 인가 후 1회만 호출한다.
 *
 * 수행 내용:
 *   - TIM4 PWM 시작 (buck 컨버터 출력, duty=0)
 *   - TIM10 1ms 타이머 시작
 *   - I2C2 / I2C3 하드웨어 리셋 (DeInit → ReInit)
 *   - INA219 센서 초기화 (블로킹 I2C)
 *   - 최초 DMA 읽기 시작
 *
 * 주의:
 *   K 토글(ON/OFF) 시에는 이 함수를 호출하지 않는다.
 *   센서·타이머를 반복 초기화하면 I2C BUSY / freeze 원인이 된다.
 * ========================================================= */
void App_Charger_HwInit(void)
{
    HAL_StatusTypeDef st_pwm;
    HAL_StatusTypeDef st_tim10;
    HAL_StatusTypeDef st_solar;
    HAL_StatusTypeDef st_batt;

    g_pending_ctrl_10ms = 0U;
    g_pending_dbg_500ms = 0U;

    g_uart2_tx_busy   = 0U;
    g_log_queue_head  = 0U;
    g_log_queue_tail  = 0U;
    g_log_queue_count = 0U;
    g_log_drop_count  = 0U;

    memset((void *)g_log_queue, 0, sizeof(g_log_queue));
    memset((void *)g_log_len,   0, sizeof(g_log_len));

    st_pwm = HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    if (st_pwm != HAL_OK)
    {
        App_Charger_LogString("TIM4 PWM start failed\r\n");
    }

    AppCharger_ApplyDuty(0.0f);

    st_tim10 = HAL_TIM_Base_Start_IT(&htim10);
    if (st_tim10 != HAL_OK)
    {
        App_Charger_LogString("TIM10 start failed\r\n");
    }

    App_Charger_LogString("\r\n");
    App_Charger_LogString("Solar charger HW init begin.\r\n");

    /* I2C 버스 강제 리셋 (최초 1회)
     * ---------------------------------------------------------
     * 이전 DMA 상태나 BUSY/ERROR를 완전히 정리한 뒤 센서를 초기화한다.
     * --------------------------------------------------------- */
    if (hi2c2.hdmarx != NULL)
    {
        (void)HAL_DMA_Abort(hi2c2.hdmarx);
    }
    (void)HAL_I2C_DeInit(&hi2c2);
    MX_I2C2_Init();

    if (hi2c3.hdmarx != NULL)
    {
        (void)HAL_DMA_Abort(hi2c3.hdmarx);
    }
    (void)HAL_I2C_DeInit(&hi2c3);
    MX_I2C3_Init();

    AppCharger_I2CScanBus(&hi2c2, "I2C2");
    AppCharger_I2CScanBus(&hi2c3, "I2C3");

    st_solar = SolarSensing_Init(&g_solar_sensor, &hi2c2, INA219_ADDR_7BIT_DEFAULT);
    g_solar_init_ok = (st_solar == HAL_OK) ? 1U : 0U;
    if (st_solar != HAL_OK)
    {
        AppCharger_PrintI2CError("SolarSensing solar init", &hi2c2);
    }
    else
    {
        App_Charger_LogString("Solar sensor init OK\r\n");
    }

    st_batt = SolarSensing_Init(&g_batt_sensor, &hi2c3, INA219_ADDR_7BIT_DEFAULT);
    g_batt_init_ok = (st_batt == HAL_OK) ? 1U : 0U;
    if (st_batt != HAL_OK)
    {
        AppCharger_PrintI2CError("SolarSensing battery init", &hi2c3);
    }
    else
    {
        App_Charger_LogString("Battery sensor init OK\r\n");
    }

    /* 최초 DMA 읽기 시작 */
    (void)SolarSensing_StartUpdateDMA(&g_solar_sensor);
    (void)SolarSensing_StartUpdateDMA(&g_batt_sensor);

    App_Charger_LogString("Solar charger HW init done.\r\n");

    /* UART6 즉시 출력 */
    {
        static char init_msg[64];
        int init_len = snprintf(init_msg, sizeof(init_msg),
                                "[CHARGER HW INIT] PV:%s BAT:%s\r\n",
                                g_solar_init_ok ? "OK" : "FAIL",
                                g_batt_init_ok  ? "OK" : "FAIL");
        if (init_len > 0)
        {
            if (huart6.gState == HAL_UART_STATE_READY)
            {
                HAL_UART_Transmit_DMA(&huart6, (uint8_t *)init_msg, (uint16_t)init_len);
            }
        }
    }
}

/* =========================================================
 * App_Charger_Init  (소프트 리셋)
 * ---------------------------------------------------------
 * K ON 토글 시 호출한다.
 *
 * 수행 내용:
 *   - 제어 pending 카운터 / 로그 큐 리셋
 *   - duty 0으로 강제
 *   - PI 제어 / 충전 상태머신 리셋
 *   - DMA 읽기 재시작 (이미 진행 중이면 무시됨)
 *
 * 수행하지 않는 내용:
 *   - I2C 리셋 / 센서 재초기화 (freeze 원인 → HwInit 에서만 수행)
 *   - TIM4 / TIM10 재시작 (이미 동작 중)
 * ========================================================= */
void App_Charger_Init(void)
{
    g_pending_ctrl_10ms = 0U;
    g_pending_dbg_500ms = 0U;

    g_uart2_tx_busy   = 0U;
    g_log_queue_head  = 0U;
    g_log_queue_tail  = 0U;
    g_log_queue_count = 0U;
    g_log_drop_count  = 0U;

    memset((void *)g_log_queue, 0, sizeof(g_log_queue));
    memset((void *)g_log_len,   0, sizeof(g_log_len));

    /* duty 강제 0 */
    AppCharger_ApplyDuty(0.0f);

    /* PI 제어 및 충전 상태머신 리셋 */
    SolarPiControl_Init(&g_ctrl);
    ChargerState_Init(&g_charger, &g_solar_sensor, &g_batt_sensor, &g_ctrl);

    /* DMA 읽기 재시작 (센서가 이미 busy면 HAL_BUSY 반환 후 무시됨) */
    (void)SolarSensing_StartUpdateDMA(&g_solar_sensor);
    (void)SolarSensing_StartUpdateDMA(&g_batt_sensor);

    App_Charger_LogString("Solar charger start.\r\n");
}



void App_Charger_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0U);

}

/* =========================================================
 * App_Charger_Task
 * ---------------------------------------------------------
 * 우선순위:
 *   1) UART kick
 *   2) 제어 task 1회 수행
 *   3) 여유가 있으면 debug log 출력
 *
 * 주의:
 * - control backlog가 있어도 한 번에 모두 처리하지 않고
 *   task 1회당 1 step만 수행해 main loop 응답성을 유지한다.
 * ========================================================= */
void App_Charger_Task(void)
{
    uint8_t run_ctrl = 0U;
    uint8_t run_dbg = 0U;

    AppCharger_UartTryStartTx();

    __disable_irq();
    if (g_pending_ctrl_10ms > 0U)
    {
        g_pending_ctrl_10ms--;
        run_ctrl = 1U;
    }
    __enable_irq();

    if (run_ctrl != 0U)
    {
        ChargerState_Run(&g_charger);
        AppCharger_ApplyDuty(g_charger.duty_last);
    }

    /* debug 출력은 control backlog가 없을 때만 수행 */
    __disable_irq();
    if ((g_pending_dbg_500ms > 0U) && (g_pending_ctrl_10ms == 0U))
    {
        g_pending_dbg_500ms--;
        run_dbg = 1U;
    }
    __enable_irq();

    if (run_dbg != 0U)
    {
        /* AppCharger_PrintStatus: vsnprintf float 인수 14개 + ISR 중첩으로
         * 스택 오버플로우 → Hard Fault 유발. UART6(SHOW_UART6_APP_CHARGER)으로
         * 충분히 모니터링 가능하므로 UART2 debug print 생략. */
    }

    AppCharger_UartTryStartTx();
}

/* =========================================================
 * App_Charger_Tick1ms
 * ---------------------------------------------------------
 * 1ms timer ISR에서 호출
 * ========================================================= */
void App_Charger_Tick1ms(void)
{
    static uint16_t cnt_ctrl = 0U;
    static uint16_t cnt_dbg  = 0U;

    cnt_ctrl++;
    if (cnt_ctrl >= APP_CHARGER_CONTROL_PERIOD_MS)
    {
        cnt_ctrl = 0U;

        if (g_pending_ctrl_10ms < APP_CHARGER_MAX_PENDING_CTRL)
        {
            g_pending_ctrl_10ms++;
        }
    }

    cnt_dbg++;
    if (cnt_dbg >= APP_CHARGER_DEBUG_PERIOD_MS)
    {
        cnt_dbg = 0U;

        if (g_pending_dbg_500ms < APP_CHARGER_MAX_PENDING_DBG)
        {
            g_pending_dbg_500ms++;
        }
    }
}

/* =========================================================
 * HAL I2C callback routing
 * ========================================================= */

void App_Charger_I2CMemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    SolarSensing_I2C_MemRxCpltCallback(hi2c);
}

void App_Charger_I2CErrorCallback(I2C_HandleTypeDef *hi2c)
{
    SolarSensing_I2C_ErrorCallback(hi2c);
}

/* =========================================================
 * SHOW_UART2_APP_CHARGER
 * ---------------------------------------------------------
 * 1초 주기, printf(UART2 blocking) 전체 상태 출력
 * 형식:
 *   [HH:MM:SS] [STATE:...][MODE:...][FAULT:...]
 *   Vpv=V Ipv=mA Ppv=W | Vbat=V Ichg=mA Pchg=W |
 *   Eff=% SOC=% Duty=  | Iref=A Icc=A Icv=A Imppt=A
 * ========================================================= */
void SHOW_UART2_APP_CHARGER(void)
{
    static uint32_t prev_tick = 0U;
    uint32_t now = HAL_GetTick();
    uint32_t total_sec;
    uint32_t hh, mm, ss;

    if ((now - prev_tick) < 1000U) return;
    prev_tick = now;

    total_sec = now / 1000U;
    hh = (total_sec / 3600U) % 100U;
    mm = (total_sec /   60U) %  60U;
    ss =  total_sec          %  60U;

    printf("[%02lu:%02lu:%02lu] [STATE:%s][MODE:%s][FAULT:%s] "
           "Vpv=%.3fV Ipv=%.1fmA Ppv=%.3fW | "
           "Vbat=%.3fV Ichg=%.1fmA Pchg=%.3fW | "
           "Eff=%.1f%% SOC=%.1f%% Duty=%.3f | "
           "Iref=%.3fA Icc=%.3fA Icv=%.3fA Imppt=%.3fA\r\n",
           (unsigned long)hh, (unsigned long)mm, (unsigned long)ss,
           ChargerState_StateString(g_charger.state),
           SolarPiControl_ModeString(g_charger.control_mode_last),
           ChargerState_PrimaryFaultString(g_charger.fault_flags),
           g_charger.solar_src_v_last,
           g_charger.solar_i_last   * 1000.0f,
           g_charger.solar_p_last,
           g_charger.batt_v_last,
           g_charger.batt_i_last    * 1000.0f,
           g_charger.batt_p_last,
           g_charger.eff_last,
           g_charger.soc_last,
           g_charger.duty_last,
           g_charger.i_ref_last,
           g_charger.i_cc_last,
           g_charger.i_cv_last,
           g_charger.i_mppt_last);
}

/* =========================================================
 * SHOW_UART6_APP_CHARGER
 * ---------------------------------------------------------
 * 1초 주기, HAL_UART_Transmit_DMA(UART6 non-blocking) 전체 상태 출력
 * - buf는 static: DMA 전송 완료까지 유지되어야 함
 * - huart6.gState 체크: 이전 전송 중이면 skip
 * - DMA2_Stream6_IRQHandler -> HAL_DMA_IRQHandler -> TC ->
 *   USART6_IRQHandler -> HAL_UART_TxCpltCallback 으로 완료 처리
 * ========================================================= */
void SHOW_UART6_APP_CHARGER(void)
{
    static uint32_t prev_tick = 0U;
    static char buf[256];
    uint32_t now = HAL_GetTick();
    uint32_t total_sec;
    uint32_t hh, mm, ss;
    int len;

    if ((now - prev_tick) < 1000U) return;
    if (huart6.gState != HAL_UART_STATE_READY) return;
    prev_tick = now;

    total_sec = now / 1000U;
    hh = (total_sec / 3600U) % 100U;
    mm = (total_sec /   60U) %  60U;
    ss =  total_sec          %  60U;

    len = snprintf(buf, sizeof(buf),
                   "[%02lu:%02lu:%02lu] [STATE:%s][MODE:%s][FAULT:%s][PV:%s][BAT:%s] "
                   "Vpv=%.3fV Ipv=%.1fmA Ppv=%.3fW | "
                   "Vbat=%.3fV Ichg=%.1fmA Pchg=%.3fW | "
                   "Eff=%.1f%% SOC=%.1f%% Duty=%.3f\r\n",
                   (unsigned long)hh, (unsigned long)mm, (unsigned long)ss,
                   ChargerState_StateString(g_charger.state),
                   SolarPiControl_ModeString(g_charger.control_mode_last),
                   ChargerState_PrimaryFaultString(g_charger.fault_flags),
                   g_solar_init_ok ? "OK" : "FAIL",
                   g_batt_init_ok  ? "OK" : "FAIL",
                   g_charger.solar_src_v_last,
                   g_charger.solar_i_last   * 1000.0f,
                   g_charger.solar_p_last,
                   g_charger.batt_v_last,
                   g_charger.batt_i_last    * 1000.0f,
                   g_charger.batt_p_last,
                   g_charger.eff_last,
                   g_charger.soc_last,
                   g_charger.duty_last);

    if (len > 0)
    {
        HAL_UART_Transmit_DMA(&huart6, (uint8_t *)buf, (uint16_t)len);
    }
}

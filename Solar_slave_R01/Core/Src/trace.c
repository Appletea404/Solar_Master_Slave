#include "trace.h"

bool initflag = 0;
Mode mode = MODE_INIT;

extern uint16_t adcValue[6];
uint16_t S1,S2,S3,S4;

int error_x;
int error_y;

uint16_t sum_array[6];
uint16_t sensor_buffer[6][FILTER_SIZE];
uint8_t filter_index = 0;

static uint8_t trace_toggle_req = 0;

void Trace_ForceInit(void)
{
		trace_toggle_req = 0;
		initflag = 0;
    mode = MODE_INIT;
}

void Trace_RequestToggle(void)
{
    trace_toggle_req = 1;
}

const char* Trace_GetStateString(void)
{
    if (mode == MODE_ACT)
        return "ACT";
    else
        return "INIT";
}

//void Trace_Task(uint8_t is_manual,
//                uint8_t is_auto,
//                uint8_t actual_speed,
//                uint8_t warning_count,
//                uint8_t has_danger,
//                uint8_t danger_latched)
//{
//    /* 1. auto에서는 무조건 init */
//    if (is_auto)
//    {
//        Trace_ForceInit();
//        Trace_Mode();
//        return;
//    }
//
//    /* 2. 주행 중이면 무조건 init */
//    if (actual_speed != 0)
//    {
//        Trace_ForceInit();
//        Trace_Mode();
//        return;
//    }
//
//    /* 3. 정지 중이어도 warning 3단계 이상 / danger / latched danger면 init */
//    if (warning_count >= 3 || has_danger || danger_latched)
//    {
//        Trace_ForceInit();
//        Trace_Mode();
//        return;
//    }
//
//    /* 4. manual + 정지 + safe 상태일 때만 D 토글 허용 */
//    if (is_manual && trace_toggle_req)
//    {
//        initflag = !initflag;
//
//        if (initflag)
//            mode = MODE_ACT;
//        else
//            mode = MODE_INIT;
//
//        trace_toggle_req = 0;
//    }
//
//    Trace_Mode();
//}

// 10개의 adc값을 모아서 개수만큼 나눈 평균값을 사용(노이즈 제거)
void Sensor_Filter()
{


//
//	S1 = adcValue[4];
//	S2 = adcValue[2];
//	S3 = adcValue[5];
//	S4 = adcValue[3];



  for(uint8_t i = 0; i < 4; i++)
  {
    sum_array[i] -= sensor_buffer[i][filter_index]; // 이전 값 제거
    sensor_buffer[i][filter_index] = adcValue[i];   // 새 값 저장
    sum_array[i] += adcValue[i];                    // 새 값 추가
  }

  filter_index++;
  if(filter_index >= FILTER_SIZE) filter_index = 0;




//   S1 = sum_array[3] / FILTER_SIZE; 4
//   S2 = sum_array[1] / FILTER_SIZE; 2
//   S3 = sum_array[4] / FILTER_SIZE; 5
//   S4 = sum_array[2] / FILTER_SIZE; 3



  S1 = sum_array[0] / FILTER_SIZE;		//1
  S2 = sum_array[1] / FILTER_SIZE;		//2
  S3 = sum_array[2] / FILTER_SIZE;		//3
  S4 = sum_array[3] / FILTER_SIZE;		//4
}


// 태양 방향 계산
void Sun_Position()
{
  int left  = S3 + S4;
  int right = S1 + S2;

  int top    = S1 + S3;
  int bottom = S2 + S4;



// == 기존 == //
//  int left  = S1 + S3;
//  int right = S2 + S4;
//
//  int top    = S1 + S2;
//  int bottom = S3 + S4;

  error_x = left - right;
  error_y = top - bottom;
}
int target_pan;
int target_tilt;



//// 목표 서보각 계산
//void Target_Update()
//{
//  if(error_x > THRESHOLD) target_pan--;
//  else if(error_x < -THRESHOLD) target_pan++;
//
//  if(error_y > THRESHOLD) target_tilt--;
//  else if(error_y < -THRESHOLD) target_tilt++;
//
//  // 최소, 최대각 설정(ARR 1000기준 CCR 20~120 동작)
//  if(target_pan < 20) target_pan = 20;
//  if(target_pan > 120) target_pan = 120;
//
//  if(target_tilt < 40) target_tilt = 40;
//  if(target_tilt > 110) target_tilt = 110;
//}
//


// 목표 서보각 계산 (원본)
void Target_Update()
{
  if(error_x > THRESHOLD) target_pan--;
  else if(error_x < -THRESHOLD) target_pan++;

  if(error_y > THRESHOLD) target_tilt++;
  else if(error_y < -THRESHOLD) target_tilt--;

  // 최소, 최대각 설정(ARR 1000기준 CCR 20~120 동작)
  if(target_pan < 20) target_pan = 20;
  if(target_pan > 120) target_pan = 120;		//수평

  if(target_tilt < 40) target_tilt = 40;		//50, 100 또는 60,90
  if(target_tilt > 110) target_tilt = 110;		//수직
}

int pan = 70;
int tilt = 70;
//int pan;
//int tilt;

// 서보모터 이동 제어
void Servo_Move()
{
  if(pan < target_pan) pan++;
  else if(pan > target_pan) pan--;

  if(tilt < target_tilt) tilt++;
  else if(tilt > target_tilt) tilt--;

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pan);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, tilt);
}

uint32_t sensor_time = 0;
uint32_t servo_time = 0;

void Trace_Init()
{
  target_pan  = 70;
  target_tilt = 70;

  // 20ms마다 서보 회전
  if(HAL_GetTick() - servo_time >= 20)
  {
    servo_time = HAL_GetTick();

    Servo_Move();
  }
}

void Trace_Act()
{
  // 50ms마다 조도 측정
  if(HAL_GetTick() - sensor_time >= 50)
  {
    sensor_time = HAL_GetTick();

    Sensor_Filter();
    Sun_Position();

    Target_Update();
  }

  // 20ms마다 서보 회전
  if(HAL_GetTick() - servo_time >= 20)
  {
    servo_time = HAL_GetTick();

    Servo_Move();
  }
}

void Trace_Mode(uint8_t mode)
{
  switch (mode) {
    case MODE_INIT:
      Trace_Init();
      break;
    case MODE_ACT:
      Trace_Act();
      break;
  }
}


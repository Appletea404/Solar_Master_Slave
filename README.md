# ☀️ HELIOS-BMS

## **1. Project Summary (프로젝트 요약)**

> **광원 추적형 태양광 충전 및 다중 센서 안전 제어를 통합한 차세대 EV 플랫폼**

**HELIOS-BMS** 는 자동차 산업의 전동화 및 SDV(Software Defined Vehicle) 전환 흐름에 대응하여, **태양광 에너지 수집, 고효율 충전 시스템, 배터리 안전 관리, 차량 주행 제어** 기능을 단일 MCU 플랫폼으로 통합한 EV 통합 제어 시스템입니다.

기존 고정 전원 기반 충전 시스템과 전압·전류·온도 중심의 BMS 구조가 가지는 한계를 극복하기 위해, 본 프로젝트에서는 다음 세 가지 기술 축을 결합하였습니다.

- **광원 추적 시스템** 을 통한 태양광 입사각 최적화 및 발전 효율 향상
- **4차 소신호 모델링 기반 PI 제어기** 와 **MPPT–CC–CV 통합 충전 알고리즘**
- **온도·가스·전압·전류 다중 센서 기반** 배터리 안전 관리 및 단계적 차량 속도 제한

또한 사용자 조작을 위한 **조이스틱 기반 수동 주행 모드** 와 **초음파 센서를 이용한 자율 주행 모드** 를 함께 구현하여 주행 기능을 확장하였으며, **Master–Slave 이중 보드 구조** 와 **CAN 통신** 을 통해 시스템 기능을 분산 제어하도록 설계하였습니다.

---

## 👥 1.1 Team & Affiliation (소속 및 팀원)

- **소속:** 대한상공회의소
- **참여자:** 김수연, 박도영, 유동원, 이석현

---

## 2. Key Features (주요 기능)

### ☀️ 광원 추적 시스템 (Solar Tracking)

- **4채널 CDS 조도 센서** 의 상대 비교 방식 기반 태양 방향 추정
- 좌우 센서 차이(`error_x`)로 **Pan**, 상하 센서 차이(`error_y`)로 **Tilt** 결정
- **Pan/Tilt 2축 서보 모터(MG996R)** 와 PWM 제어를 통한 패널 각도 실시간 추적
- 센서 출력단에 **저역통과 필터(LPF, fc ≒ 15.9 Hz)** 와 **OP-AMP 버퍼(LM358N)** 적용으로 노이즈 제거 및 임피던스 매칭

### 🔋 Buck Converter 기반 태양광 충전 시스템

- **Buck Converter 기반 전용 충전 모듈** 설계로 태양광 전력을 배터리 충전 전압으로 변환
- 태양전지–배터리 결합 시스템의 비선형 동특성을 반영한 **4차 소신호 모델링** 수행
- **MPPT–CC–CV 통합 충전 제어** 구조로 입력 조건과 배터리 상태에 따른 자동 모드 전환
- **PI 제어기 이중 루프 구조** : 전류 루프 1.5 kHz 교차 / 전압 루프 1 Hz 교차로 루프 간 간섭 방지
- 비선형 시뮬레이션 및 실제 STM32 구현을 통해 **평균 약 81~82 % 효율** 달성 (상용 MPPT 모듈 약 78 % 대비 향상)

### 🛡️ 다중 센서 기반 배터리 안전 관리 시스템 (BMS)

- **온도(NTC Thermistor)·가스(MQ-135)·전압·전류(INA219, I2C)** 4종 센서 기반 실시간 계측
- 기존 BMS 의 한계인 **열폭주 초기 오프개싱(Off-gassing) 현상** 을 가스 센서로 조기 감지
- 센서별 **SAFE / WARNING / DANGER** 3단계 상태 분류 및 Electrical 그룹 통합 판단
- WARNING 개수에 따른 **단계적 속도 제한 (80 % → 60 % → 40 % → 0 %)** 및 ramp 방식 점진 반영
- **DANGER latch 구조** 를 통한 이상 원인 기록 및 UART DMA 기반 1초 주기 로그 출력

### 🚗 차량 주행 제어 시스템 (Manual / Auto)

- **수동 주행 모드** : Bluetooth(HC-05) + STM32F411 Blackpill 조이스틱 기반 1바이트 ASCII 명령 제어
- **자율 주행 모드** : 3채널 HC-SR04 초음파 센서 + 4-state FSM(SCAN / PIVOT / BACK / STOP)
- TIM3 입력 캡처를 이용한 ECHO 펄스 측정 및 거리 환산 ($Distance = EchoPulse / 58$)
- **L298N 모터 드라이버** 기반 PWM 속도 제어 및 좌·우 센서 거리 비교를 통한 회피 방향 결정

### 📡 CAN 통신 기반 분산 제어 구조

- **Master Board ↔ Slave Board** 간 CAN 통신을 통한 기능 분산
- **MCP2515 + TJA1050** 구성, 8 MHz 크리스탈 기준 **125 kbps** 비트레이트
- 비트 타이밍 세그먼트(Sync / Prop / Phase1 / Phase2 = 1 / 3 / 8 / 4 Tq) 설계 및 **샘플 포인트 75 %** 적용
- 표준 데이터 프레임 기반 애플리케이션 프로토콜 정의 (CMD / VALUE 2바이트)

---

## 🛠 3. Tech Stack (기술 스택)

### 3.1 Language (사용 언어)

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)

### 3.2 Development Environment (개발 환경)

| **STM32CubeIDE** | **STM32CubeMX** | **VS Code** |
| :---: | :---: | :---: |
| 펌웨어 빌드 및 디버깅 | 핀 / 클럭 / 주변장치 설정 | 코드 편집 및 Git 관리 |

### 3.3 Simulation & Analysis (제어 해석 및 시뮬레이션)

| **Google Colab** | **Falstad Circuit Simulator** |
| :---: | :---: |
| 4차 소신호 모델 / Bode Plot 분석 | Buck Converter 회로 동작 검증 |

### 3.4 Debugging Tools (실험 및 디버깅 환경)

| **Moserial** | **Serial Bluetooth Terminal** |
| :---: | :---: |
| UART 로그 모니터링 | Bluetooth 기반 명령 송수신 테스트 |

### 3.5 Collaboration Tools (협업 도구)

![Github](https://img.shields.io/badge/GitHub-100000?style=for-the-badge&logo=github&logoColor=white)
![Discord](https://img.shields.io/badge/Discord-7289DA?style=for-the-badge&logo=discord&logoColor=white)
![Notion](https://img.shields.io/badge/Notion-000000?style=for-the-badge&logo=notion&logoColor=white)

---

## 📂 4. Project Structure (프로젝트 구조)

### 4.1 Project Tree (프로젝트 트리)

```
HELIOS_BMS/
├── Solar_master_R01/                # Master Board 펌웨어 (차량 제어 + BMS)
│   ├── Core/
│   │   ├── Inc/                     # 헤더 파일 (.h)
│   │   └── Src/
│   │       ├── main.c               # 시스템 초기화 및 메인 루프
│   │       ├── app_bms.c            # BMS 통합 태스크 (App_Bms_Task)
│   │       ├── bms_sensor.c         # INA219 / NTC / MQ-135 센서 측정
│   │       ├── bms_safety.c         # SAFE / WARNING / DANGER 상태 판단
│   │       ├── safe_drive.c         # 단계적 속도 제한 (ramp 적용)
│   │       ├── car_drive.c          # Manual / Auto 주행 FSM
│   │       ├── ultrasonic.c         # HC-SR04 거리 측정 (TIM3 입력 캡처)
│   │       ├── can_master.c         # MCP2515 CAN 송수신 (SPI 기반)
│   │       └── uart_log.c           # UART6 DMA 기반 로그 출력
│   └── Solar_master_R01.ioc         # STM32CubeMX 설정 파일
│
├── Solar_slave_R01/                 # Slave Board 펌웨어 (태양광 + 충전)
│   ├── Core/
│   │   ├── Inc/
│   │   └── Src/
│   │       ├── main.c               # 시스템 초기화 및 메인 루프
│   │       ├── solar_tracking.c     # 4채널 CDS 기반 Pan/Tilt 제어
│   │       ├── servo_pwm.c          # MG996R 서보 PWM 출력
│   │       ├── buck_charger.c       # MPPT-CC-CV 통합 충전 제어
│   │       ├── pi_controller.c      # 전류·전압 이중 루프 PI 제어기
│   │       ├── adc_sensing.c        # 입출력 전압·전류 ADC 계측
│   │       └── can_slave.c          # MCP2515 CAN 수신
│   └── Solar_slave_R01.ioc          # STM32CubeMX 설정 파일
│
├── Remote/                          # 원격 조종기 (STM32F411 Blackpill)
│   └── Core/
│       └── Src/
│           ├── main.c               # 조이스틱 ADC + 푸쉬버튼 입력
│           ├── joystick.c           # ADC1 연속 변환 모드 샘플링
│           └── bt_uart.c            # HC-05 Bluetooth UART 송신
│
├── 공모전파일/                        # 공모전 제출 자료 (보고서, BOM, 영상)
│
├── .gitignore                       # Git 제외 항목 설정
└── README.md                        # 프로젝트 개요 문서
```

### 4.2 Hardware Block Diagram (전체 하드웨어 블록도)

> 전체 시스템 하드웨어 블록도 이미지를 추가하세요.
> `![HW_BlockDiagram](images/HW_BLOCK_DIAGRAM.png)`

본 시스템은 **Master Board** 와 **Slave Board** 의 이중 제어 구조로 설계되었으며, 두 보드는 **CAN 통신** 을 통해 연결되어 시스템 기능을 분산 제어합니다.

| **Master Board** | **Slave Board** |
| :---: | :---: |
| 차량 주행 제어 + 배터리 안전 관리 | 광원 추적 + 태양광 충전 |
| HC-05 / 조이스틱 명령 수신 | 4채널 CDS + 서보 Pan/Tilt 제어 |
| 다중 센서 기반 BMS 판단 | Buck Converter + MPPT-CC-CV |
| L298N 모터 드라이버 PWM 제어 | 입출력 전압·전류 ADC 계측 |

### 4.3 Master Board FSM

> Master Board 상태 머신 다이어그램을 추가하세요.
> `![Master_FSM](images/Master_FSM.png)`

```
INIT  ─────►  IDLE  ◄────►  DRIVE (Manual / Auto)
                │
                └────►  FORCE_STOP   (배터리 DANGER 또는 정지 명령)
```

### 4.4 Slave Board FSM

> Slave Board 상태 머신 다이어그램을 추가하세요.
> `![Slave_FSM](images/Slave_FSM.png)`

```
INIT  ─────►  CAN_IDLE  ─────►  TRACKING / CHARGING (CC / CV / MPPT)
```

### 4.5 CAN Application Protocol (CAN 데이터 필드)

| **CAN ID** | **DLC** | **Byte[0] CMD** | **Byte[1] VALUE** | **기능** |
| :---: | :---: | :---: | :---: | :---: |
| 0x123 | 2 | 0x10 | 0x00 / 0x01 | 태양광 트래킹 ON / OFF |
| 0x123 | 2 | 0x11 | 0x00 / 0x01 | 태양광 충전 ON / OFF |
| 0x123 | 2 | 0x12 | 0x01 / 0x02 | 강제 정지 / 강제 재초기화 |
| 0x123 | 2 | 0x13 | 0x00 / 0x01 | 자율주행 ON / OFF |

---

## 🧮 5. Control Theory & Implementation (제어 이론 및 구현)

### 5.1 4차 소신호 모델링 (4th-order Small-signal Modeling)

태양전지–배터리 결합 시스템은 입력(태양전지의 비선형 I-V 특성)과 출력(배터리 CC-CV 특성)이 동시에 변하는 비선형 시스템입니다. 단순 Buck Converter 모델로는 이러한 동특성을 정확히 반영할 수 없으므로, 입력과 출력의 결합 특성을 고려한 **4차 소신호 모델** 을 구성하였습니다.

**상태 변수 정의:**

$$x = [v_{pv}, \ v_{in}, \ i_L, \ v_o]^T$$

| 상태 변수 | 의미 |
| :---: | :--- |
| $v_{pv}$ | 태양전지 전압 |
| $v_{in}$ | 스위칭 입력 전압 |
| $i_L$ | 인덕터 전류 |
| $v_o$ | 출력 전압 |

입력단을 $v_{pv}$ 와 $v_{in}$ 으로 분리하여 태양전지의 비선형 특성과 스위칭 동작이 결합된 입력 동특성을 반영하였습니다.

### 5.2 MPPT–CC–CV 통합 충전 제어

전류 지령은 **CC, CV, MPPT 조건 중 가장 작은 값** 으로 결정되며, 입력 조건과 배터리 상태에 따라 충전 모드가 자동 전환됩니다.

$$i_{ref} = \min(I_{CC}, \ I_{CV}, \ I_{MPPT})$$

### 5.3 PI 제어기 설계 결과

| **구분** | **fc** | **fz** | **Kp** | **Ki** | **PM** |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **전류 루프** | 1500 Hz | 300 Hz | 0.457 | 861.9 | 63.1° |
| **전압 루프** | 1 Hz | 0.2 Hz | 6.54 | 8.21 | 168.6° |

전류 루프는 내부 루프로서 빠른 전류 추종을 위해 높은 대역폭을 가지며, 전압 루프는 외부 루프로서 충분히 느리게 동작하도록 설계하여 **루프 간 간섭을 방지** 하였습니다.

### 5.4 BMS 안전 상태 판단 기준

| **센싱 데이터** | **상태** | **기준** |
| :---: | :---: | :---: |
| **TEMP** | SAFE | < 45 ℃ |
| | WARNING | ≥ 45 ℃ |
| | DANGER | ≥ 55 ℃ |
| **GAS** | SAFE | ΔADC < 150 |
| | WARNING | ΔADC ≥ 150 |
| | DANGER | ΔADC ≥ 400 |
| **CURRENT** | SAFE | < 1700 mA |
| | WARNING | ≥ 1700 mA |
| | DANGER | ≥ 2500 mA |
| **VOLTAGE** | UNDER WARNING | ≤ 8.0 V |
| | UNDER DANGER | ≤ 5.0 V |
| | OVER WARNING | ≥ 13.0 V |
| | OVER DANGER | ≥ 13.5 V |

### 5.5 단계적 속도 제한 정책

| **시스템 상태** | **최대 속도 제한** |
| :---: | :---: |
| SAFE | 80 % |
| WARNING 1개 | 60 % |
| WARNING 2개 | 40 % |
| WARNING 3개 또는 DANGER | 0 % (차량 정지) |

속도 제한 값은 **400 ms 주기로 갱신** 되며 **ramp 방식으로 점진적으로 반영** 됩니다.

### 5.6 수동 주행 명령 (Remote Command Map)

| **명령 코드** | **동작** | **속도** |
| :---: | :---: | :---: |
| 'F' | 전진 | 100 % |
| 'Q' | 전진 (저속) | 50 % |
| 'B' | 후진 | 100 % |
| 'W' | 후진 (저속) | 50 % |
| 'L' | 좌 회전 | 100 % |
| 'E' | 좌 회전 (저속) | 50 % |
| 'R' | 우 회전 | 100 % |
| 'T' | 우 회전 (저속) | 50 % |
| 'S' | 정지 | 0 % |
| 'A' | 자율 / 수동 모드 전환 | - |
| 'P' | 강제 정지 / 재초기화 토글 | - |

---

## 🏁 6. Final Product & Demonstration (완성품 및 시연)

### 6.1 Final Product (완성품)

> 완성품 사진을 아래 형식으로 추가하세요.

| **전면 (Front)** | **상단 (Top)** | **후면 (Back)** |
| :---: | :---: | :---: |
| `<img src="images/Front.jpg" width="250">` | `<img src="images/Top.jpg" width="250">` | `<img src="images/Back.jpg" width="250">` |

| **광원 추적 시스템** | **Buck Converter 충전 모듈** | **BMS 감지 모듈** |
| :---: | :---: | :---: |
| `<img src="images/SolarTracker.jpg" width="250">` | `<img src="images/BuckCharger.jpg" width="250">` | `<img src="images/BMS_Module.jpg" width="250">` |

### 6.2 Demonstration (시연 영상)

> 시연 영상 링크를 아래 형식으로 추가하세요.

```html
<a href="유튜브_링크" target="_blank">
  <img src="images/youtube.jpg" alt="Watch Demo Video" width="300" />
</a>
```

**Demonstration Check List:**

| **항목** | **검증 내용** |
| :---: | :--- |
| **01. Temp Stress Test** | 고온 환경에서의 온도 센서 응답 및 속도 제한 동작 검증 |
| **02. Gas Stress Test** | MQ-135 센서 baseline 대비 가스 농도 변화 감지 검증 |
| **03. Electrical Stress Test** | INA219 기반 과전류 / 과전압 / 저전압 보호 동작 검증 |
| **04. BMS Stress Test** | 다중 WARNING 발생 시 단계적 속도 제한 및 latch 구조 검증 |
| **05. Solar Charge System Simulation** | CC → MPPT → CV 모드 전환 및 충전 효율 검증 |
| **06. Solar Trace Simulation** | 4채널 CDS 센서 기반 Pan/Tilt 추적 동작 검증 |

---

## 7. Troubleshooting (문제 해결 기록)

### 7.1 태양전지–배터리 결합 시스템의 비선형 동특성

🔍 **Issue (문제 상황)**

- 태양전지의 비선형 I-V 특성과 배터리의 CC–CV 특성이 동시에 작용하는 환경에서, 단순 Buck Converter 모델 기반 제어기로는 충전 전류·전압이 안정적으로 수렴하지 않음

❓ **Analysis (원인 분석)**

- 입력단(태양전지)과 출력단(배터리)이 상호 영향을 미치는 비선형 결합 시스템임에도 불구하고, 표준 2차 소신호 모델은 이 결합 특성을 반영하지 못함
- 최대 전력점(MPP)이 일사량에 따라 시간에 따라 이동하는 점을 동적 모델에 포함시키지 못함

❗ **Action (해결 방법)**

- 입력단을 $v_{pv}$ 와 $v_{in}$ 으로 분리한 **4차 소신호 모델** 구성
- SW ON / SW OFF 상태별 미분방정식 도출 후 듀티비 $d$ 를 이용한 **평균화 비선형 모델** 유도
- 정상상태 동작점 주변 선형화를 통해 상태공간 행렬 $A$, $B$ 도출 및 PI 제어기 설계

✅ **Result (결과)**

- Bode Plot 분석을 통해 전류 루프 PM 63.1°, 전압 루프 PM 168.6° 의 안정적인 위상 여유 확보
- 비선형 시뮬레이션에서 약 0.8 A 로 안정적 수렴 및 모드 전환 정상 동작 확인

---

### 7.2 MPPT / CC / CV 모드 전환 충돌

🔍 **Issue (문제 상황)**

- 충전 진행 중 입력 조건(일사량) 변화와 배터리 상태 변화가 동시에 발생할 때, MPPT / CC / CV 모드 간 전환이 매끄럽지 않고 진동 발생

❓ **Analysis (원인 분석)**

- 각 모드가 독립적으로 전류 지령을 생성하는 경우 모드 경계에서 지령 값이 충돌하여 진동 유발
- 단순 우선순위 기반 모드 선택은 입력·배터리 조건이 동시에 변할 때 부드러운 전환을 보장하지 못함

❗ **Action (해결 방법)**

- 전류 지령을 **min 함수 기반 통합 구조** 로 재설계: $i_{ref} = \min(I_{CC}, I_{CV}, I_{MPPT})$
- 가장 작은 전류 지령이 항상 지배하도록 하여 모드 전환을 자동·연속적으로 수행

✅ **Result (결과)**

- 약 3.7 V 배터리 조건 실험에서 CC → MPPT → CC → CV 모드 전환이 끊김 없이 수행됨을 확인
- **평균 충전 효율 약 81~82 %** 달성 (상용 MPPT 모듈 약 78 % 대비 향상)

---

### 7.3 기존 BMS 의 열폭주 조기 감지 한계

🔍 **Issue (문제 상황)**

- 전압·전류·온도 중심의 기존 BMS 구조로는 배터리 열폭주(Thermal Runaway) 의 초기 단계인 **오프개싱(Off-gassing) 현상** 을 사전 감지하지 못함
- 인천 청라 전기차 화재 사례와 같이, BMS 이상 경고가 발생하지 않은 상태에서 화재로 진행될 수 있는 위험 존재

❓ **Analysis (원인 분석)**

- 셀 내부에서 가스가 방출되는 단계에서는 외부 온도·전압·전류에 유의미한 변화가 발생하지 않을 수 있음
- 전기적 이상 신호가 나타날 시점에는 이미 열폭주가 본격화된 상태일 가능성이 큼

❗ **Action (해결 방법)**

- 기존 감시 구조에 **MQ-135 가스 센서** 를 추가하여 다중 안전 감지 시스템 구성
- 시스템 시작 시 일정 시간 동안 baseline 평균을 형성하고, 현재 측정값과의 차이($\Delta ADC$) 로 가스 농도 변화 판단
- 노이즈 감소를 위한 **이동 평균 필터** 적용 및 SAFE / WARNING / DANGER 단계별 임계값 정의

✅ **Result (결과)**

- 가스 농도 변화 조기 감지 후 **단계적 속도 제한** (80 % → 60 % → 40 % → 0 %) 적용 가능
- DANGER 발생 원인을 latch 구조에 기록하여 사후 로그 분석 가능

---

### 7.4 조도 센서 출력 노이즈 및 임피던스 매칭 문제

🔍 **Issue (문제 상황)**

- CDS 조도 센서의 ADC 입력 신호에 고주파 노이즈가 섞여 Pan/Tilt 제어 시 서보 떨림(jitter) 발생
- 센서 출력 임피던스가 ADC 입력 임피던스와 맞지 않아 측정값이 불안정

❓ **Analysis (원인 분석)**

- CDS 센서와 10 kΩ 풀업 저항으로 구성된 전압 분배 회로의 출력 임피던스가 높음
- ADC 샘플링 시 입력 캐패시턴스 충전이 충분히 이루어지지 않아 측정 오차 발생
- 외부 EMI 및 전원 노이즈가 신호선에 직접 유입

❗ **Action (해결 방법)**

- **저역통과 필터(LPF)** 추가하여 차단 주파수 $f_c \approx 15.9 \ Hz$ 이상의 노이즈 제거
- **LM358N OP-AMP 를 전압 버퍼($A_v = 1$) 로 구성** 하여 임피던스 매칭 및 위상 유지
- 12비트 ADC 로 정량화하여 정밀 조도 비교 가능하도록 구성

✅ **Result (결과)**

- 4채널 CDS 센서의 안정적인 ADC 측정값 확보
- 좌·우 / 상·하 센서 차이 비교를 통한 부드러운 Pan/Tilt 추적 동작 확인

---

### 7.5 CAN 통신 비트 타이밍 정확도 확보

🔍 **Issue (문제 상황)**

- Master ↔ Slave 간 CAN 통신 초기 구현 시 메시지 손실 및 ACK 미수신 발생
- 비트 타이밍이 부정확하여 노드 간 동기화 실패

❓ **Analysis (원인 분석)**

- MCP2515 의 비트 타이밍 레지스터(CNF1, CNF2, CNF3) 설정값이 8 MHz 크리스탈 기준 125 kbps 목표와 정합되지 않음
- 샘플 포인트가 적절한 위치에 있지 않아 신호 안정 영역에서 샘플링되지 못함

❗ **Action (해결 방법)**

- Time Quantum 기준으로 비트 시간 분할: $T_q = 0.5 \ \mu s$, **Total = 16 Tq**
- 세그먼트 배분: Sync(1) + Prop(3) + Phase1(8) + Phase2(4) = 16 Tq
- **샘플 포인트 75 %** 적용하여 비트 시간의 75 % 지점에서 안정적 샘플링 수행
- MCP2515 가 ACK 미수신 시 자동 재전송하도록 설정 + 소프트웨어 재시도 로직 이중화

✅ **Result (결과)**

- 안정적인 125 kbps CAN 통신 확보
- Master ↔ Slave 간 표준 데이터 프레임(0x123 ID, DLC=2) 기반 명령 송수신 정상 동작

---

## 📦 8. Bill of Materials (주요 부품 리스트)

> 구매처: 디바이스마트 / 상세 BOM 은 `공모전파일/` 디렉토리 참조

### 8.1 태양광 충전 시스템

| 분류 | 부품명 | 비고 |
| :---: | :---: | :---: |
| 태양전지 | D165x165-3 (6 V 4.5 W) | - |
| 전력 소자 | IRF9540N / 1N5822 / 2N2222A | P-MOSFET / Diode / BJT |
| 인덕터 | RING COIL 11파이 (220 µH) | - |
| 캐패시터 | E/C 16 V 220 µF / 50 V 10 µF / 16 V 470 µF | - |
| 전류 센서 | INA219 (SZH-SSBH-075) | I2C |
| 배터리 | UB848 (18650 3.7 V 2600 mAh) | - |

### 8.2 광원 추적 시스템

| 분류 | 부품명 | 비고 |
| :---: | :---: | :---: |
| 서보 모터 | TowerPro MG996R | Pan / Tilt |
| 조도 센서 | CdS Cell GL5549 | 4 채널 |
| 전원 | SRS-155 (3 V 150 mA) | - |
| OP-AMP | LM358N | 전압 버퍼 |
| Buck Module | LM2596HV | - |

### 8.3 배터리 안전 관리 시스템

| 분류 | 부품명 | 비고 |
| :---: | :---: | :---: |
| 온도 센서 | NTC-10KGJG | 10 kΩ Thermistor |
| 가스 센서 | MQ-135 (SZH-SSBH-038) | - |
| 전류 센서 | INA219 (SZH-SSBH-075) | I2C |
| OP-AMP | LM358N | LPF 버퍼 |

### 8.4 차량 주행 제어 시스템

| 분류 | 부품명 | 비고 |
| :---: | :---: | :---: |
| MCU | NUCLEO-F411RE | Master Board |
| 모터 드라이버 | L298N (SZH-EK001) | 2 A |
| 초음파 센서 | HC-SR04 (SZH-USBC-004) | 3 채널 |
| Bluetooth | HC-05 / HC-06 | - |
| CAN 컨트롤러 | MCP2515 + TJA1050 | SPI |

### 8.5 무선 조종부

| 분류 | 부품명 | 비고 |
| :---: | :---: | :---: |
| MCU | STM32F411 BlackPill | Adafruit ada-4877 |
| 조이스틱 | PS2 듀얼 로커 모듈 (ELB070682) | - |
| Bluetooth | HC-05 (SZH-EK069) | - |

---

## 📚 9. References (참고문헌)

1. International Energy Agency (IEA), "Korea Energy Profile," [link](https://www.iea.org/countries/korea/energy-mix)
2. U.S. Energy Information Administration (EIA), "Oil flows through the Strait of Hormuz," [link](https://www.eia.gov/todayinenergy/detail.php?id=61002)
3. MDPI, "Photovoltaic Modeling: A Comprehensive Analysis of the I–V Characteristic Curve," 2024, [link](https://www.mdpi.com/2071-1050/16/1/432)
4. 김일송, 「PSIM과 MATLAB을 활용한 전력변환 제어기 설계」, 홍릉과학출판사, 2023.
5. CTA, "CES 2026 Innovation Awards – Vehicle Solar Module," [link](https://www.ces.tech/ces-innovation-awards/2026/solarstic-injection-molded-vehicle-solar-module/)
6. Microchip Technology, "Photovoltaic I–V Curve and Maximum Power Point (MPP)," [link](https://ww1.microchip.com/downloads/en/appnotes/00001521a.pdf)
7. Texas Instruments, "Understanding Buck Power Stages in Switchmode Power Supplies (SLVA057)," [link](https://www.ti.com/lit/an/slva057/slva057.pdf)
8. 머니투데이, "청라 전기차 화재 1년.. BMS 이상 경고 없어," [link](https://www.mt.co.kr/estate/2025/09/11/2025091109535276893)

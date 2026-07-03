# uC/OS-III 기반 임베디드 이진수 시각화 시스템

숫자를 입력하면 2진수로 변환하여 LED로 표현하고, 2색 LED로 짝수/홀수를 시각화하는 교육용 임베디드 시스템

### 소개

본 프로젝트는 STM32F429 마이크로컨트롤러와 uC/OS-III 실시간 운영체제(RTOS)를 기반으로 한 "이진수 시각화 교육 시스템"입니다. 사용자가 UART로 숫자(0~7)를 입력하면 3비트 이진수로 변환하여 3개의 LED로 시각적으로 표현하고, 동시에 2색 LED를 통해 입력된 숫자의 홀짝성을 직관적으로 나타냅니다.

LED를 단순한 On/Off 스위치가 아닌, 2진수의 본질을 표현하는 매개체로 재해석하여, 추상적인 이진수 개념을 물리적으로 체험할 수 있도록 설계했습니다.

### 개발 배경 및 목적

- **디지털 교육의 중요성**: 이진수와 디지털 논리 개념을 물리적 LED로 구체화하여 학습 효과 향상
- **STEM 교육 도구 필요성**: 초중고 정보교육 실습 도구, 대학 임베디드 교육의 이론-실습 연결 매개체
- **실시간 시스템 학습**: 다중 태스크 환경에서의 동기화 및 통신 기법 체득

### 시스템 아키텍처

4개의 태스크로 구성되며, uC/OS-III의 우선순위 기반 스케줄링을 사용합니다.

| 태스크 | 우선순위 | 역할 |
|---|---|---|
| USART Task | 5 | 사용자 입력 처리, UART 통신, 명령어 해석 및 실행 |
| LED Task | 6 | 메시지 큐로부터 명령 수신 및 LED 상태 변경 |
| Button Task | 7 | 버튼 상태 검사 및 LED 상태 USART 전송 |
| AppTaskStart | - | 시스템 초기화 및 관리 |

태스크 간 통신은 메시지 큐(Message Queue)로 구현하며, LED 상태 변수 동기화는 **Critical Section**으로 보호합니다.

### 하드웨어 구성

- **보드**: NUCLEO-F439ZI
- **LED 출력**: 3개의 In-built 단색 LED(빨/파/노, GPIOB), 1개의 2색 LED(GPIOC Pin 0, 3)
- **버튼 입력**: 1개의 푸시 버튼(GPIOC Pin 13, Active Low, 보드 내장)
- **UART 통신**: USART3 (PD8-RX, PD9-TX, 115200 baud)

### 동작 시나리오

1. 전원 인가 후 USART를 통해 사용자 명령 입력
2. 명령어 처리:
   - `0` ~ `7`: 입력값을 3비트 이진 패턴으로 LED1~3에 표시 (예: `5` → `0b101` → LED1, LED3 ON)
   - 짝수 번호: 2색 LED 초록색 ON / 홀수 번호: 2색 LED 빨간색 ON
   - `reset`: 모든 LED OFF
3. 버튼을 누르면 현재 LED1~3 상태를 USART로 출력

### 기술적 구현

- `USART_Config()`, `STM_Nucleo_COMInit()`로 USART3 초기화
- `send_string()`으로 비동기 UART 송신
- `GPIO_Init()`으로 버튼 및 LED 초기 설정 (LED: Push-Pull 2MHz, 버튼: Pull-Down)
- `Set_2ColorLED()`로 2색 LED 상태 동기화 (Common Cathode 방식)
- 모든 태스크는 `OSTaskCreate()`로 생성 (Stack Size 및 우선순위 개별 설정)
- 버튼 입력은 Edge 감지 + 100ms 디바운싱으로 노이즈 제거
- UART 수신은 인터럽트 기반으로 효율적 처리

### 기술 스택

- **RTOS**: uC/OS-III (선점형 멀티태스킹)
- **MCU**: STM32F429 (NUCLEO-F439ZI)
- **언어**: C
- **라이브러리**: STM32 HAL

### 역할 분담

| 이름 | 담당 |
|---|---|
| 박소영 | 태스크 구조 설계, uC/OS-III 초기화 및 스케줄링, 버튼 입력 처리 |
| 따다소 | GPIO 설정 및 LED 제어, UART 통신 구현, 메시지 큐 구현 |

### 결론 및 기대효과

- **실시간 멀티태스킹 구현**: uC/OS-III의 우선순위 기반 스케줄링, 메시지 큐, Critical Section을 활용해 버튼 입력·LED 제어·UART 통신을 독립적으로 운영하며 안정적인 실시간 처리를 구현
- **하드웨어-소프트웨어 통합 설계**: GPIO, UART, 인터럽트를 통합해 임베디드 시스템 핵심 요소를 결합
- **교육적 활용 가치**: 추상적인 2진수 개념을 LED 시각화로 직관적으로 학습, 홀짝 개념을 색상으로 구분해 이해도 향상

---

### Overview

This project is a "Binary Number Visualization Education System" built on the STM32F429 microcontroller running the uC/OS-III real-time operating system (RTOS). When a user inputs a number (0–7) via UART, it is converted into a 3-bit binary pattern displayed across 3 LEDs, while a bicolor LED simultaneously indicates whether the number is even or odd.

Rather than treating LEDs as simple on/off switches, this project reinterprets them as a medium for expressing the essence of binary numbers, allowing abstract binary concepts to be experienced physically.

### Background & Purpose

- **Importance of digital education**: Makes abstract binary/digital logic concepts tangible through physical LEDs
- **Need for STEM education tools**: Serves as a hands-on tool for K-12 informatics education and bridges theory and practice in university embedded systems courses
- **Real-time systems learning**: Builds understanding of task synchronization and communication in multitasking environments

### System Architecture

The system consists of 4 tasks, scheduled using uC/OS-III's priority-based scheduler.

| Task | Priority | Role |
|---|---|---|
| USART Task | 5 | Handles user input, UART communication, command parsing and execution |
| LED Task | 6 | Receives commands from the message queue and updates LED states |
| Button Task | 7 | Checks button state and sends LED status via USART |
| AppTaskStart | - | System initialization and management |

Inter-task communication is implemented via a **Message Queue**, and LED state variable synchronization is protected using a **Critical Section**.

### Hardware Configuration

- **Board**: NUCLEO-F439ZI
- **LED Output**: 3 built-in single-color LEDs (red/blue/yellow, GPIOB), 1 bicolor LED (GPIOC Pin 0, 3)
- **Button Input**: 1 push button (GPIOC Pin 13, Active Low, on-board)
- **UART Communication**: USART3 (PD8-RX, PD9-TX, 115200 baud)

### Operating Scenario

1. Upon power-up, the user enters commands via USART
2. Command handling:
   - `0`–`7`: converts the input to a 3-bit binary pattern displayed on LED1–3 (e.g., `5` → `0b101` → LED1, LED3 ON)
   - Even numbers: bicolor LED turns green / Odd numbers: bicolor LED turns red
   - `reset`: turns off all LEDs
3. Pressing the button outputs the current LED1–3 status via USART

### Technical Implementation

- USART3 initialized via `USART_Config()` and `STM_Nucleo_COMInit()`
- Asynchronous UART transmission via `send_string()`
- Button and LED setup via `GPIO_Init()` (LED: Push-Pull 2MHz, Button: Pull-Down)
- Bicolor LED state synchronized via `Set_2ColorLED()` (Common Cathode)
- All tasks created via `OSTaskCreate()` with individually configured stack size and priority
- Button input handled via edge detection with 100ms debouncing
- UART reception handled via interrupts for efficiency

### Tech Stack

- **RTOS**: uC/OS-III (preemptive multitasking)
- **MCU**: STM32F429 (NUCLEO-F439ZI)
- **Language**: C
- **Library**: STM32 HAL

### Team Roles

| Name | Role |
|---|---|
| Park So-Young | Task structure design, uC/OS-III initialization & scheduling, button input handling |
| Thadar Soe | GPIO setup and LED control, UART communication, message queue implementation |

### Conclusion & Expected Impact

- **Real-time multitasking**: Achieved stable real-time processing of button input, LED control, and UART communication using uC/OS-III's priority-based scheduling, message queues, and critical sections
- **Hardware-software integration**: Combined GPIO (LED/button), UART, and interrupts to experience the organic collaboration between hardware and software
- **Educational value**: Makes abstract binary concepts easy to understand through intuitive LED visualization, with color-coded parity for enhanced comprehension

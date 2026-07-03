/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                       IAR Development Kits
*                                              on the
*
*                                    STM32F429II-SK KICKSTART KIT
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : YS
*                 DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <includes.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx.h"
#include "bsp.h"
#include "os.h"
#include "cpu.h"
#include <stdbool.h>
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_TASK_EQ_0_ITERATION_NBR              16u

#define COMn 1
#define NUCLEO_COM1                      USART3
#define NUCLEO_COM1_CLK                  RCC_APB1Periph_USART3

#define NUCLEO_COM1_TX_PIN               GPIO_Pin_9
#define NUCLEO_COM1_TX_GPIO_PORT         GPIOD
#define NUCLEO_COM1_TX_GPIO_CLK          RCC_AHB1Periph_GPIOD
#define NUCLEO_COM1_TX_SOURCE            GPIO_PinSource9
#define NUCLEO_COM1_TX_AF                GPIO_AF_USART3

#define NUCLEO_COM1_RX_PIN               GPIO_Pin_8
#define NUCLEO_COM1_RX_GPIO_PORT         GPIOD
#define NUCLEO_COM1_RX_GPIO_CLK          RCC_AHB1Periph_GPIOD
#define NUCLEO_COM1_RX_SOURCE            GPIO_PinSource8
#define NUCLEO_COM1_RX_AF                GPIO_AF_USART3

#define NUCLEO_COM1_IRQn                 USART3_IRQn
#define TASK_STK_SIZE 256

#define LED_RED_PIN    GPIO_Pin_0   // PC0 (빨간색)
#define LED_GREEN_PIN  GPIO_Pin_3   // PC3 (초록색)
#define LED_2COLOR_PORT GPIOC       // GPIOC 사용
#define LED_2COLOR_CLK  RCC_AHB1Periph_GPIOC

volatile bool reset_flag = false;

// 태스크 스택 및 TCB 선언
#define USART_TASK_PRIO 5
#define LED_TASK_PRIO 6
#define BUTTON_TASK_PRIO 7  // 버튼 태스크 우선순위
#define BUTTON_PIN GPIO_Pin_13
#define BUTTON_PORT GPIOC
#define BUTTON_CLK RCC_AHB1Periph_GPIOC

#define  MSG_QUEUE_SIZE  10

static OS_TCB AppTaskStartTCB;
static CPU_STK AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB UsartTaskTCB;
static CPU_STK UsartTaskStk[TASK_STK_SIZE];

static OS_TCB LedTaskTCB;
static CPU_STK LedTaskStk[TASK_STK_SIZE];

// 버튼 태스크 관련 추가
static OS_TCB ButtonTaskTCB;
static CPU_STK ButtonTaskStk[TASK_STK_SIZE];

OS_Q   LedMsgQueue;

/*
*********************************************************************************************************
*                                            TYPES DEFINITIONS
*********************************************************************************************************
*/
typedef enum {
   TASK_500MS,
   TASK_1000MS,
   TASK_2000MS,

   TASK_N
}task_e;
typedef struct
{
   CPU_CHAR* name;
   OS_TASK_PTR func;
   OS_PRIO prio;
   CPU_STK* pStack;
   OS_TCB* pTcb;
}task_t;

typedef enum
{
    COM1 = 0,
    COM2 = 1
} COM_TypeDef;

typedef enum {
    CMD_OFF = 0,
    CMD_ON
} LedCmd_t;

volatile LedCmd_t LED1CMD = CMD_OFF;
volatile LedCmd_t LED2CMD = CMD_OFF;
volatile LedCmd_t LED3CMD = CMD_OFF;

USART_TypeDef* COM_USART[COMn] = { NUCLEO_COM1 };
GPIO_TypeDef* COM_TX_PORT[COMn] = { NUCLEO_COM1_TX_GPIO_PORT };
GPIO_TypeDef* COM_RX_PORT[COMn] = { NUCLEO_COM1_RX_GPIO_PORT };
const uint32_t COM_USART_CLK[COMn]     = { NUCLEO_COM1_CLK };
const uint32_t COM_TX_PORT_CLK[COMn]   = { NUCLEO_COM1_TX_GPIO_CLK };
const uint32_t COM_RX_PORT_CLK[COMn]   = { NUCLEO_COM1_RX_GPIO_CLK };
const uint16_t COM_TX_PIN[COMn]        = { NUCLEO_COM1_TX_PIN };
const uint16_t COM_RX_PIN[COMn]        = { NUCLEO_COM1_RX_PIN };
const uint16_t COM_TX_PIN_SOURCE[COMn] = { NUCLEO_COM1_TX_SOURCE };
const uint16_t COM_RX_PIN_SOURCE[COMn] = { NUCLEO_COM1_RX_SOURCE };
const uint16_t COM_TX_AF[COMn]         = { NUCLEO_COM1_TX_AF };
const uint16_t COM_RX_AF[COMn]         = { NUCLEO_COM1_RX_AF };

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void  AppTaskStart          (void     *p_arg);
static  void  AppTaskCreate         (void);
static  void  AppObjCreate          (void);

static void AppTask_500ms(void *p_arg);
static void AppTask_1000ms(void *p_arg);
static void AppTask_2000ms(void *p_arg);

static void Setup_Gpio(void);
static void USART_Config(void);
void STM_Nucleo_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct);
void send_string(const char *str);
static void UsartTask(void *p_arg);
static void LedTask(void *p_arg);
static void ButtonTask(void *p_arg);  // 버튼 태스크 함수 추가
void Set_2ColorLED(bool red_on, bool green_on);

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
/* ----------------- APPLICATION GLOBALS -------------- */
static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB       Task_500ms_TCB;
static  OS_TCB       Task_1000ms_TCB;
static  OS_TCB       Task_2000ms_TCB;

static  CPU_STK  Task_500ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_1000ms_Stack[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  Task_2000ms_Stack[APP_CFG_TASK_START_STK_SIZE];
int count=0;
task_t cyclic_tasks[TASK_N] = {
   {"Task_500ms" , AppTask_500ms,  0, &Task_500ms_Stack[0] , &Task_500ms_TCB},
   {"Task_1000ms", AppTask_1000ms, 0, &Task_1000ms_Stack[0], &Task_1000ms_TCB},
   {"Task_2000ms", AppTask_2000ms, 0, &Task_2000ms_Stack[0], &Task_2000ms_TCB},
};

int main(void)
{
    OS_ERR  err;

    /* Basic Init */
    RCC_DeInit();
    Setup_Gpio();

    Set_2ColorLED(true, false);
    /* BSP Init */
    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    CPU_Init();                                                 /* Initialize the uC/CPU Services                       */
    Mem_Init();                                                 /* Initialize Memory Management Module                  */
    Math_Init();                                                /* Initialize Mathematical Module                       */


    /* OS Init */
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSTaskCreate((OS_TCB       *)&AppTaskStartTCB,              /* Create the start task                                */
                 (CPU_CHAR     *)"App Task Start",
                 (OS_TASK_PTR   )AppTaskStart,
                 (void         *)0u,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK      *)&AppTaskStartStk[0u],
                 (CPU_STK_SIZE  )AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0u,
                 (OS_TICK       )0u,
                 (void         *)0u,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);

   OSStart(&err);   /* Start multitasking (i.e. give control to uC/OS-III). */

   (void)&err;

   return (0u);
}

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR  err;

    (void)p_arg;

    BSP_Init();
    BSP_Tick_Init();
    USART_Config();

    OSQCreate(&LedMsgQueue,           // 큐 객체
                  "LED Command Queue",    // 이름 (디버깅용)
                  MSG_QUEUE_SIZE,         // 큐 크기
                  &err);

    // USART 및 LED Task 생성
    OSTaskCreate(&UsartTaskTCB,
                 "USART Task",
                 UsartTask,
                 NULL,
                 USART_TASK_PRIO,
                 &UsartTaskStk[0],
                 TASK_STK_SIZE / 10,
                 TASK_STK_SIZE,
                 0,
                 0,
                 NULL,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

    OSTaskCreate(&LedTaskTCB,
                 "LED Task",
                 LedTask,
                 NULL,
                 LED_TASK_PRIO,
                 &LedTaskStk[0],
                 TASK_STK_SIZE / 10,
                 TASK_STK_SIZE,
                 0,
                 0,
                 NULL,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &err);

    // 버튼 태스크 생성 추가
        OSTaskCreate(&ButtonTaskTCB,
                     "Button Task",
                     ButtonTask,
                     NULL,
                     BUTTON_TASK_PRIO,
                     &ButtonTaskStk[0],
                     TASK_STK_SIZE / 10,
                     TASK_STK_SIZE,
                     0,
                     0,
                     NULL,
                     OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                     &err);

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    APP_TRACE_DBG(("Creating Application Kernel Objects\n\r"));
    AppObjCreate();

    APP_TRACE_DBG(("Creating Application Tasks\n\r"));
    AppTaskCreate();
}

static void AppTask_500ms(void *p_arg)
{
    OS_ERR  err;
    BSP_LED_On(1);
    while (DEF_TRUE) {
       BSP_LED_Toggle(1);
        OSTimeDlyHMSM(0u, 0u, 0u, 500u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
       count++;
    }
}

static void AppTask_1000ms(void *p_arg)
{
    OS_ERR err;
    BSP_LED_On(2);
    while (DEF_TRUE) {
        BSP_LED_Toggle(2);
        OSTimeDlyHMSM(0u, 0u, 1u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

static void AppTask_2000ms(void *p_arg)
{
    OS_ERR  err;
    BSP_LED_On(3);
    while (DEF_TRUE) {
       BSP_LED_Toggle(3);
        OSTimeDlyHMSM(0u, 0u, 2u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

static void AppTaskCreate (void)
{
    OS_ERR err;

    OSTaskCreate(
        (OS_TCB       *)&Task_1000ms_TCB,
        (CPU_CHAR     *)"AppTask_1000ms",
        (OS_TASK_PTR  )AppTask_1000ms,
        (void         *)0u,
        (OS_PRIO      )0u,
        (CPU_STK      *)&Task_1000ms_Stack[0u],
        (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE / 10u,
        (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE,
        (OS_MSG_QTY   )0u,
        (OS_TICK      )0u,
        (void         *)0u,
        (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
        (OS_ERR       *)&err);
}

static  void  AppObjCreate (void)
{
}

static void Setup_Gpio(void)
{
   GPIO_InitTypeDef led_init = {0};
   GPIO_InitTypeDef button_init = {0};
   GPIO_InitTypeDef gpio_init;

   // LED용 GPIO 클럭 활성화 (GPIOB)
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

   // 버튼용 GPIO 클럭 활성화 (GPIOC)
   RCC_AHB1PeriphClockCmd(BUTTON_CLK, ENABLE);

   // SYSCFG 클럭 활성화 (필요한 경우)
   RCC_AHB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

   RCC_AHB1PeriphClockCmd(LED_2COLOR_CLK, ENABLE);

   // LED GPIO 설정
   led_init.GPIO_Mode   = GPIO_Mode_OUT;
   led_init.GPIO_OType  = GPIO_OType_PP;
   led_init.GPIO_Speed  = GPIO_Speed_2MHz;
   led_init.GPIO_PuPd   = GPIO_PuPd_NOPULL;
   led_init.GPIO_Pin    = GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14;
   GPIO_Init(GPIOB, &led_init);

   // 버튼 GPIO 설정
   button_init.GPIO_Mode = GPIO_Mode_IN;
   button_init.GPIO_PuPd = GPIO_PuPd_DOWN;
   button_init.GPIO_Pin = BUTTON_PIN;
   GPIO_Init(BUTTON_PORT, &button_init);

   gpio_init.GPIO_Pin   = LED_RED_PIN | LED_GREEN_PIN;
       gpio_init.GPIO_Mode  = GPIO_Mode_OUT;
       gpio_init.GPIO_OType = GPIO_OType_PP;
       gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
       gpio_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
       GPIO_Init(LED_2COLOR_PORT, &gpio_init);

       // Turn off both LEDs initially
       GPIO_WriteBit(LED_2COLOR_PORT, LED_RED_PIN, Bit_RESET);
       GPIO_WriteBit(LED_2COLOR_PORT, LED_GREEN_PIN, Bit_RESET);
}

static void USART_Config(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    STM_Nucleo_COMInit(COM1, &USART_InitStructure);
}

void STM_Nucleo_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable GPIO clock
    RCC_AHB1PeriphClockCmd(COM_TX_PORT_CLK[COM] | COM_RX_PORT_CLK[COM], ENABLE);

    // Enable UART clock
    RCC_APB1PeriphClockCmd(COM_USART_CLK[COM], ENABLE);

    // TX 핀 설정
    GPIO_PinAFConfig(COM_TX_PORT[COM], COM_TX_PIN_SOURCE[COM], COM_TX_AF[COM]);
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = COM_TX_PIN[COM];
    GPIO_Init(COM_TX_PORT[COM], &GPIO_InitStructure);

    // RX 핀 설정
    GPIO_PinAFConfig(COM_RX_PORT[COM], COM_RX_PIN_SOURCE[COM], COM_RX_AF[COM]);
    GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[COM];
    GPIO_Init(COM_RX_PORT[COM], &GPIO_InitStructure);

    // USART 설정
    USART_Init(COM_USART[COM], USART_InitStruct);

    // USART 활성화
    USART_Cmd(COM_USART[COM], ENABLE);
}

void send_string(const char *str)
{
    while (*str) {
        while (USART_GetFlagStatus(NUCLEO_COM1, USART_FLAG_TXE) == RESET);
        USART_SendData(NUCLEO_COM1, *str++);
    }
}

static void UsartTask(void *p_arg) {
    CPU_CHAR rx_buf[32];
    int idx = 0;
    char c;

    (void)p_arg;

    while (DEF_TRUE) {
        while (USART_GetFlagStatus(NUCLEO_COM1, USART_FLAG_RXNE) == RESET) {
            OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, NULL);
        }

        c = USART_ReceiveData(NUCLEO_COM1);
        USART_SendData(NUCLEO_COM1, c); // echo

        if (c == '\r' || c == '\n') {
            rx_buf[idx] = '\0';
            idx = 0;

            OS_ERR err;
            CPU_SR cpu_sr = 0;
            OS_CRITICAL_ENTER();

            if (strcmp(rx_buf, "reset") == 0) {
                uint8_t cmd = 0xFE;  // 리셋 명령 (특수 값)
                OSQPost(&LedMsgQueue,
                        (void *)&cmd,
                        sizeof(cmd),
                        OS_OPT_POST_FIFO,
                        &err);
                send_string("\n\rReset all LEDs\n\r");
                Set_2ColorLED(false, false);
            }
            else if (strlen(rx_buf) == 1 && rx_buf[0] >= '0' && rx_buf[0] <= '7') {
                uint8_t cmd = rx_buf[0] - '0';

                if (cmd % 2 == 0) {
                    Set_2ColorLED(false, true);  // Even: Green
                } else {
                    Set_2ColorLED(true, false);  // Odd: Red
                }

                OSQPost(&LedMsgQueue,
                        (void *)&cmd,
                        sizeof(cmd),
                        OS_OPT_POST_FIFO,
                        &err);
                send_string("\r\nLED pattern set\r\n");
            }
            else {
                send_string("\r\nInvalid command. Use 0-7 or 'reset'\r\n");
            }

            OS_CRITICAL_EXIT();
        }
        else if (idx < sizeof(rx_buf) - 1) {
            rx_buf[idx++] = c;
        }
    }
}


static void LedTask(void *p_arg) {
    OS_ERR err;
    uint8_t cmd;
    OS_MSG_SIZE msg_size;
    static bool reset_flag = false;

    (void)p_arg;

    while (DEF_TRUE) {
        void *p_msg = OSQPend(&LedMsgQueue,
                              0,
                              OS_OPT_PEND_NON_BLOCKING,
                              &msg_size,
                              NULL,
                              &err);

        if (err == OS_ERR_NONE && p_msg != NULL) {
            cmd = *(uint8_t *)p_msg;

            if (cmd == 0xFE) {
                reset_flag = true;
            }
            else if (cmd <= 0x07) {
                reset_flag = false;

                // Set LEDs
                if (cmd & 0x01) {
                    BSP_LED_On(1);
                    LED1CMD = CMD_ON;
                } else {
                    BSP_LED_Off(1);
                    LED1CMD = CMD_OFF;
                }

                if (cmd & 0x02) {
                    BSP_LED_On(2);
                    LED2CMD = CMD_ON;
                } else {
                    BSP_LED_Off(2);
                    LED2CMD = CMD_OFF;
                }

                if (cmd & 0x04) {
                    BSP_LED_On(3);
                    LED3CMD = CMD_ON;
                } else {
                    BSP_LED_Off(3);
                    LED3CMD = CMD_OFF;
                }
            }
        }

        if (reset_flag) {
            BSP_LED_Off(1);
            BSP_LED_Off(2);
            BSP_LED_Off(3);
            LED1CMD = CMD_OFF;
            LED2CMD = CMD_OFF;
            LED3CMD = CMD_OFF;
            reset_flag = false;
        }

        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);
    }
}


void Set_2ColorLED(bool red_on, bool green_on) {
    // Common Cathode: HIGH = ON, LOW = OFF
    GPIO_WriteBit(LED_2COLOR_PORT, LED_RED_PIN,   red_on   ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(LED_2COLOR_PORT, LED_GREEN_PIN, green_on ? Bit_SET : Bit_RESET);
}

static void ButtonTask(void *p_arg)
{
    OS_ERR err;
    uint8_t last_button_state = 0;
    uint8_t current_button_state;
    CPU_SR  cpu_sr = 0u;
    char status_msg[50];

    (void)p_arg;

    while (DEF_TRUE) {
        // 버튼 상태 읽기
        current_button_state = GPIO_ReadInputDataBit(BUTTON_PORT, BUTTON_PIN);

        // 버튼이 눌렸을 때 (상승 에지 감지)
        if (current_button_state && !last_button_state) {
           OS_CRITICAL_ENTER();
            LedCmd_t cmd1 = LED1CMD;
            LedCmd_t cmd2 = LED2CMD;
            LedCmd_t cmd3 = LED3CMD;
            OS_CRITICAL_EXIT();

            // LED 상태 문자열 생성
            snprintf(status_msg, sizeof(status_msg),
                    "Current LED Status: LED1-%s, LED2-%s, LED3-%s\n\r",
                    cmd1 == CMD_ON ? "ON" : "OFF",
                    cmd2 == CMD_ON ? "ON" : "OFF",
                    cmd3 == CMD_ON ? "ON" : "OFF");

            // 시리얼로 상태 전송
            send_string(status_msg);

            // 디바운스를 위한 딜레이
            OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_TIME_DLY, &err);
        }

        last_button_state = current_button_state;

        // 태스크 주기적 실행을 위한 딜레이
        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_DLY, &err);
    }
}

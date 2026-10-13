/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define QUEUE_SIZE   11U    // 環形緩衝 11 格，犧牲一格 → 最多排 10 人
#define DEBOUNCE_MS  300U   // 按鈕去彈跳時間
#define RX_RING_SIZE 64U    // UART 收字的環形緩衝大小（Lab4 是 256）
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

/* USER CODE BEGIN PV */
static uint16_t queue[QUEUE_SIZE];          // 排隊名單（存號碼）
static uint8_t  qHead = 0U;                 // 下一個抽號要放的位置
static uint8_t  qTail = 0U;                 // 下一個要叫的位置
static uint16_t nextTicket = 1U;            // 下一張號碼牌
static volatile uint8_t ticketFlag = 0U;    // 中斷登記：有人按抽號
static uint32_t lastTicketTick = 0U;        // 上次抽號的時間（去彈跳用）
static uint16_t currentNumber = 0U;         // 正在叫的號碼，0 = 目前沒有

/* UART 收指令用（跟 Lab4 一樣：中斷收字放進 rxRing，主迴圈再組成一行） */
static uint8_t rxByte;                      // 中斷每次收 1 個字放這裡
static volatile uint8_t rxRing[RX_RING_SIZE];
static volatile uint8_t rxHead = 0U;        // 中斷寫入的位置
static volatile uint8_t rxTail = 0U;        // 主迴圈讀取的位置
static char lineBuffer[16];                 // 組成一行指令
static uint8_t lineLength = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void UART_Print(const char *msg);
static uint8_t QueueCount(void);
static uint8_t QueuePush(uint16_t number);
static void UpdateLeds(void);
static void TakeTicket(void);
static uint8_t QueuePop(uint16_t *number);
static void CallNext(void);
static void ListQueue(void);
static void ExecuteCommand(const char *cmd);
static void ProcessReceivedByte(uint8_t data);
static void RxRingPush(uint8_t data);
static uint8_t RxRingPop(uint8_t *data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 透過 UART 傳一段文字給電腦（Lab4 的 HAL_UART_Transmit 包成函式） */
static void UART_Print(const char *msg)
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), 100U);
}

/* 目前排隊人數：head 和 tail 的距離 */
static uint8_t QueueCount(void)
{
  return (uint8_t)((qHead + QUEUE_SIZE - qTail) % QUEUE_SIZE);
}

/* 抽號：把號碼放進 head，滿了回傳 0（跟 Lab4 的 RingPush 一樣） */
static uint8_t QueuePush(uint16_t number)
{
  uint8_t next = (uint8_t)((qHead + 1U) % QUEUE_SIZE);
  if (next == qTail) return 0U;      // 下一格撞到 tail → 滿了
  queue[qHead] = number;
  qHead = next;
  return 1U;
}

/* 燈號：綠燈 = 有人正在被叫號，黃燈 = 有人在排隊 */
static void UpdateLeds(void)
{
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,
                    (currentNumber != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5,
                    (QueueCount() > 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 客人抽號：放進名單，告訴他前面有幾個人 */
static void TakeTicket(void)
{
  char msg[64];
  uint8_t ahead = QueueCount();      // 放進去之前的人數 = 前面有幾人

  if (QueuePush(nextTicket) == 0U)
  {
    UART_Print("Queue is full, please wait.\r\n");
    return;
  }
  snprintf(msg, sizeof(msg), "Ticket #%u taken, %u people ahead.\r\n",
           (unsigned int)nextTicket, (unsigned int)ahead);
  UART_Print(msg);
  nextTicket++;
  UpdateLeds();
}

/* 叫號：從 tail 拿出最早抽號的人，空的回傳 0 */
static uint8_t QueuePop(uint16_t *number)
{
  if (qHead == qTail) return 0U;     // head 等於 tail → 沒人排隊
  *number = queue[qTail];
  qTail = (uint8_t)((qTail + 1U) % QUEUE_SIZE);
  return 1U;
}

/* next：叫下一位 */
static void CallNext(void)
{
  char msg[64];
  uint16_t number;

  if (QueuePop(&number) == 0U)
  {
    currentNumber = 0U;
    UART_Print("No one is waiting.\r\n");
  }
  else
  {
    currentNumber = number;
    snprintf(msg, sizeof(msg), "Now serving #%u (%u waiting).\r\n",
             (unsigned int)currentNumber, (unsigned int)QueueCount());
    UART_Print(msg);
  }
  UpdateLeds();
}

/* list：列出正在叫的號碼和排隊名單（從 tail 走到 head） */
static void ListQueue(void)
{
  char msg[128];
  int len;
  uint8_t i;

  if (currentNumber != 0U)
    len = snprintf(msg, sizeof(msg), "Serving: #%u | Waiting:", (unsigned int)currentNumber);
  else
    len = snprintf(msg, sizeof(msg), "Serving: none | Waiting:");

  if (QueueCount() == 0U)
  {
    len += snprintf(msg + len, sizeof(msg) - len, " (empty)");
  }
  for (i = qTail; i != qHead; i = (uint8_t)((i + 1U) % QUEUE_SIZE))
  {
    len += snprintf(msg + len, sizeof(msg) - len, " #%u", (unsigned int)queue[i]);
  }
  snprintf(msg + len, sizeof(msg) - len, "\r\n");
  UART_Print(msg);
}

/* 判斷店員打的指令（跟 Lab4 的 ExecuteCommand 一樣用 strcmp） */
static void ExecuteCommand(const char *cmd)
{
  char msg[48];

  if (strcmp(cmd, "next") == 0)
  {
    CallNext();
  }
  else if (strcmp(cmd, "list") == 0)
  {
    ListQueue();
  }
  else if (strcmp(cmd, "skip") == 0)
  {
    if (currentNumber == 0U)
    {
      UART_Print("No one is being served.\r\n");
    }
    else
    {
      snprintf(msg, sizeof(msg), "#%u skipped.\r\n", (unsigned int)currentNumber);
      UART_Print(msg);
      CallNext();                    // 過號後直接叫下一位
    }
  }
  else if (cmd[0] == '\0')
  {
    /* 空白行，不處理 */
  }
  else
  {
    UART_Print("Unknown command. Use: next / list / skip\r\n");
  }
}

/* 一個字一個字組成一行，看到換行才執行（跟 Lab4 的 ProcessReceivedByte 一樣）
   不同的終端機按 Enter 送出的可能是 CR、LF 或 CR+LF，所以兩種都當成「一行結束」 */
static void ProcessReceivedByte(uint8_t data)
{
  if ((data == '\r') || (data == '\n'))
  {
    if (lineLength > 0U)             // CR+LF 會連續來兩個，第二個是空行就跳過
    {
      lineBuffer[lineLength] = '\0';
      ExecuteCommand(lineBuffer);
      lineLength = 0U;
    }
  }
  else if (lineLength < sizeof(lineBuffer) - 1U)
  {
    lineBuffer[lineLength++] = (char)data;
  }
}

/* 收字的環形緩衝（跟 Lab4 的 RingPush / RingPop 一樣） */
static void RxRingPush(uint8_t data)
{
  uint8_t next = (uint8_t)((rxHead + 1U) % RX_RING_SIZE);
  if (next == rxTail) return;        // 滿了就丟掉
  rxRing[rxHead] = data;
  rxHead = next;
}

static uint8_t RxRingPop(uint8_t *data)
{
  if (rxHead == rxTail) return 0U;
  *data = rxRing[rxTail];
  rxTail = (uint8_t)((rxTail + 1U) % RX_RING_SIZE);
  return 1U;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 開機自我測試：綠 → 黃 → 紅 各亮 0.3 秒，確認三顆燈接線正確 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);    // 綠
  HAL_Delay(300);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);    // 黃
  HAL_Delay(300);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);    // 紅
  HAL_Delay(300);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);

  /* 歡迎訊息 */
  UART_Print("\r\n=== Smart Queue System ===\r\n");
  UART_Print("Press PC9 to take a number.\r\n");
  UART_Print("Commands: next / list / skip\r\n");

  /* 開始用中斷接收第一個字（Lab4 一樣） */
  if (HAL_UART_Receive_IT(&hlpuart1, &rxByte, 1U) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 主迴圈處理抽號：中斷只登記，這裡才真的做事 */
    if (ticketFlag)
    {
      ticketFlag = 0U;
      /* 去彈跳：距離上次超過 300 ms，而且按鈕現在真的是按下（0）才算數 */
      if ((HAL_GetTick() - lastTicketTick >= DEBOUNCE_MS) &&
          (HAL_GPIO_ReadPin(TICKET_BTN_GPIO_Port, TICKET_BTN_Pin) == GPIO_PIN_RESET))
      {
        lastTicketTick = HAL_GetTick();
        TakeTicket();
      }
    }

    /* 主迴圈處理店員指令：把中斷收到的字拿出來組成一行 */
    uint8_t data;
    while (RxRingPop(&data) != 0U)
    {
      ProcessReceivedByte(data);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : STOP_BTN_Pin */
  GPIO_InitStruct.Pin = STOP_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(STOP_BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC5 PC6 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : TICKET_BTN_Pin */
  GPIO_InitStruct.Pin = TICKET_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TICKET_BTN_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* 按鈕中斷：只登記，馬上離開（ISR 要短） */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == TICKET_BTN_Pin)
  {
    ticketFlag = 1U;
  }
}

/* UART 收到 1 個字：放進 rxRing，再約下一個字（Lab4 一樣） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != LPUART1) return;
  RxRingPush(rxByte);
  HAL_UART_Receive_IT(huart, &rxByte, 1U);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

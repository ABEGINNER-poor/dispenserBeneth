/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "cmsis_os.h"
#include "lwip.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "usbd_cdc_if.h"
#include "lwip/netif.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

SRAM_HandleTypeDef hsram3;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FSMC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM7_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
void Check_Network_Status(void);
void Debug_Packet_Reception_Chain(void);
void Debug_LWIP_Protocol_Stack(void);
void Debug_ARP_Table(void);
void Debug_Network_Interface(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_FSMC_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_Delay(100);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 7999;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 65535;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);

  /*Configure GPIO pin : PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  // PHY 复位
      /*HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET); // 拉低
      HAL_Delay(20); // 延时 >10ms
      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);   // 拉高*/
  /* USER CODE END MX_GPIO_Init_2 */
}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{

  /* USER CODE BEGIN FSMC_Init 0 */

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */

  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM3 memory initialization sequence
  */
  hsram3.Instance = FSMC_NORSRAM_DEVICE;
  hsram3.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram3.Init */
  hsram3.Init.NSBank = FSMC_NORSRAM_BANK3;
  hsram3.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram3.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram3.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram3.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram3.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram3.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram3.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram3.Init.WriteOperation = FSMC_WRITE_OPERATION_DISABLE;
  hsram3.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram3.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
  hsram3.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram3.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  hsram3.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  /* Timing */
  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 255;
  Timing.BusTurnAroundDuration = 15;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;
  /* ExtTiming */

  if (HAL_SRAM_Init(&hsram3, &Timing, NULL) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FSMC_Init 2 */

  /* USER CODE END FSMC_Init 2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief 检查网络状态
 */
void Check_Network_Status(void)
{
    char msg[128];
    extern ETH_HandleTypeDef heth;
    extern struct netif gnetif;
    
    // 1. 检查ETH初始化状态
    snprintf(msg, sizeof(msg), "ETH gState: %d\r\n", heth.gState);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    // 2. 检查PHY链路状态
    uint32_t phy_reg = 0;
    if (HAL_ETH_ReadPHYRegister(&heth, 0x00, PHY_BSR, &phy_reg) == HAL_OK) {
        bool link_up = (phy_reg & PHY_LINKED_STATUS) != 0;
        snprintf(msg, sizeof(msg), "PHY BSR: 0x%04lX, Link: %s\r\n", 
                 phy_reg & 0xFFFF, link_up ? "UP" : "DOWN");
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    } else {
        CDC_Transmit_FS((uint8_t*)"PHY Read Failed\r\n", 17);
    }
    
    // 3. 检查LWIP netif状态
    if (netif_is_up(&gnetif)) {
        if (netif_is_link_up(&gnetif)) {
            snprintf(msg, sizeof(msg), "LWIP: UP, IP: %d.%d.%d.%d\r\n",
                     (int)((gnetif.ip_addr.addr >> 0) & 0xFF),
                     (int)((gnetif.ip_addr.addr >> 8) & 0xFF),
                     (int)((gnetif.ip_addr.addr >> 16) & 0xFF),
                     (int)((gnetif.ip_addr.addr >> 24) & 0xFF));
        } else {
            snprintf(msg, sizeof(msg), "LWIP: Interface UP, but Link DOWN\r\n");
        }
    } else {
        snprintf(msg, sizeof(msg), "LWIP: Interface DOWN\r\n");
    }
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    // 4. 检查DMA状态
    uint32_t dma_status = ETH->DMASR;
    snprintf(msg, sizeof(msg), "DMA Status: 0x%08lX\r\n", dma_status);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    CDC_Transmit_FS((uint8_t*)"--- Network Check Complete ---\r\n", 33);
}

/**
 * @brief 调试数据包接收链路 - 专门排查ping包为什么到不了ICMP层
 */
void Debug_Packet_Reception_Chain(void)
{
    char msg[128];
    extern ETH_HandleTypeDef heth;
    extern struct netif gnetif;
    
    CDC_Transmit_FS((uint8_t*)"\r\n=== PING包接收链路调试 ===\r\n", 32);
    
    // 1. 检查ETH DMA接收状态
    uint32_t dma_sr = ETH->DMASR;
    snprintf(msg, sizeof(msg), "DMA Status: 0x%08lX\r\n", dma_sr);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    // 检查ETH是否启动
    uint32_t maccr = ETH->MACCR;
    snprintf(msg, sizeof(msg), "ETH MACCR: 0x%08lX\r\n", maccr);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    if (maccr & (1 << 2)) {  // RE - Receiver Enable
        CDC_Transmit_FS((uint8_t*)"✅ ETH接收已启用\r\n", 21);
    } else {
        CDC_Transmit_FS((uint8_t*)"❌ ETH接收未启用！\r\n", 23);
    }
    
    if (maccr & (1 << 3)) {  // TE - Transmitter Enable  
        CDC_Transmit_FS((uint8_t*)"✅ ETH发送已启用\r\n", 21);
    } else {
        CDC_Transmit_FS((uint8_t*)"❌ ETH发送未启用！\r\n", 23);
    }
    
    // 检查DMA接收相关位
    if (dma_sr & (1 << 6)) {  // RI - Receive Interrupt
        CDC_Transmit_FS((uint8_t*)"✅ DMA接收中断标志置位\r\n", 28);
    } else {
        CDC_Transmit_FS((uint8_t*)"❌ DMA接收中断标志未置位\r\n", 30);
    }
    
    if (dma_sr & (1 << 7)) {  // RU - Receive Buffer Unavailable
        CDC_Transmit_FS((uint8_t*)"⚠️ 接收缓冲区不可用\r\n", 24);
    }
    
    // 2. 检查ETH MAC接收状态
    uint32_t mac_sr = ETH->MACSR;
    snprintf(msg, sizeof(msg), "MAC Status: 0x%08lX\r\n", mac_sr);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    // 3. 检查接收描述符状态 (跳过具体地址检查)
    CDC_Transmit_FS((uint8_t*)"RX Desc: Configured\r\n", 21);
    
    // 4. 检查ETH错误状态
    uint32_t eth_error = HAL_ETH_GetError(&heth);
    if (eth_error != HAL_ETH_ERROR_NONE) {
        snprintf(msg, sizeof(msg), "❌ ETH Error: 0x%08lX\r\n", eth_error);
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    } else {
        CDC_Transmit_FS((uint8_t*)"✅ ETH Error: None\r\n", 21);
    }
    
    // 5. 检查网络接口基本信息
    snprintf(msg, sizeof(msg), "LWIP netif name: %c%c\r\n", gnetif.name[0], gnetif.name[1]);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    snprintf(msg, sizeof(msg), "LWIP MTU: %u\r\n", gnetif.mtu);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    CDC_Transmit_FS((uint8_t*)"💡 建议: 在HAL_ETH_RxCpltCallback中加断点\r\n", 48);
    CDC_Transmit_FS((uint8_t*)"💡 建议: 在ethernetif_input中加断点\r\n", 38);
    
    CDC_Transmit_FS((uint8_t*)"--- 接收链路调试完成 ---\r\n", 29);
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN 5 */
  char usb_msg[128];
  uint32_t cycle_counter = 0;
  uint32_t network_check_counter = 0;

  // 等待 USB 枚举就绪
  osDelay(2000);
  
  CDC_Transmit_FS((uint8_t*)"=== STM32 Network Test Started ===\r\n", 37);

  for(;;)
  {
    cycle_counter++;
    
    // 每5个周期（10秒）执行一次网络状态检查
    if (++network_check_counter >= 5) {
        network_check_counter = 0;
        
        snprintf(usb_msg, sizeof(usb_msg), "\r\n=== Network Check #%lu ===\r\n", cycle_counter / 5);
        CDC_Transmit_FS((uint8_t*)usb_msg, strlen(usb_msg));
        
        Check_Network_Status();
        
        // 添加ping包接收链路调试
        Debug_Packet_Reception_Chain();
    }
    
    // 发送心跳消息
    snprintf(usb_msg, sizeof(usb_msg), "Heartbeat #%lu - ping接收调试中...\r\n", cycle_counter);
    CDC_Transmit_FS((uint8_t*)usb_msg, strlen(usb_msg));
    
    osDelay(2000); // 2秒间隔
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM14 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM14)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

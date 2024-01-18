/*
 * Project		: ZephC LTE Gateway
 * Version		: 1.0
 * Revision		: 1.0
 *
 * Author		: REDWOLF DiGiTAL [C. Worakan]
 */

#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <string.h>

#include "msgProcess.h"
#include "GNSSprocess.h"
#include "EXITinti.h"
#include "LTEdriver.h"





char textBuffer[125];


char Rx1Buff[Rx1Buff_Size];
char Rx2Buff[Rx2Buff_Size];
char dataComm_mainBuff[dataComm_MainBuff_S];
char lteComm_MainBuff[lteComm_MainBuff_S];


char ENDBYTE[1];
char COMPID[1];



TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_rx;

// custom HandleTypeDef
SysTimer_HandleTypeDef sysCounter;
SysFlag_HandleTypeDef sysFlag;
GNSS_HandleTypeDef GNSS;
Sensor_HandleTypeDef SENSOR;


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM4_Init(void);



int main(void) {

  sysValinit();
  sensorValInit();

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM4_Init();


  // TIMER4 START
  HAL_TIM_Base_Start_IT(&htim4);


  // DMA LTE
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t *)Rx2Buff, Rx2Buff_Size);
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

  //DMA commMaster
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)Rx1Buff, Rx1Buff_Size);
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);



  // INTERRUPT
  initEXIT();


  //initLTE();






/*
 *  ===============================================================================
 *
 *  _____ _____ _____ _____    _____ _____ _____ _____ _____ _____ _____ _____
 * |     |  _  |     |   | |  |  _  | __  |     |   __| __  |  _  |     |   __|
 * | | | |     |-   -| | | |  |   __|    -|  |  |  |  |    -|     | | | |__   |
 * |_|_|_|__|__|_____|_|___|  |__|  |__|__|_____|_____|__|__|__|__|_|_|_|_____|
 *
 *  ===============================================================================
 */

  // Wait LTE module boot
  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_SET);			// BUSY 1
  HAL_GPIO_WritePin(GPIOB, ONLINE, GPIO_PIN_RESET);		// ONLINE 0
  while(sysCounter.main_ms_counter < 500);				// Wait MCU boot
  // Initialize LTE module
  SerialDebug("[MCU] -> Wait LTE boot 30sec.\r\n");
  while(sysCounter.main_ms_counter < LTEbootTime);		// Wait LTE module boot
  initLTE();											// Start init LTE module
  // Config PDP context
  //initPDP(1);

  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);		// BUSY 0






  while(1) {
  // ErrorHandle

  RS485Handle :
	while(intterruptEvent_Flag == 1) {									// Get RTS Signal
	  SerialDebug("[MCU] -> Get RTS\r\n");
	  sensorValInit();													// Clear buffer
	  memset(COMPID, 0x00, sizeof(COMPID));
	  memset(ENDBYTE, 0x00, sizeof(ENDBYTE));

	  // Ping to server



	  // Generate pulse 5ms.
	  SerialDebug("[MCU] -> Send RDY\r\n");
	  HAL_GPIO_WritePin(GPIOB, RDY, GPIO_PIN_SET);
	  HAL_Delay(5);
	  HAL_GPIO_WritePin(GPIOB, RDY, GPIO_PIN_RESET);

	  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_SET);						// BUSY !!!!
	  SerialDebug("[MCU] -> Wait data\r\n");

	  // Wait DMA put data to buffer
	  sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
	  while(*dataComm_mainBuff == '\0') {
		  // Timeout conditions
		  if(sysCounter.main_ms_counter == 0) {
			  sysCounter.prev_msgTimeOut = 0;
		  }

		  if((sysCounter.main_ms_counter - sysCounter.prev_msgTimeOut) >= 1000) {
			  SerialDebug("[RS485] -> Timeout\r\n");
			  SendData_RS485((char*) 'F');
			  intterruptEvent_Flag = 0;
			  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);
			  sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
			  goto RS485Handle;
		  }
	  }
	  sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;

	  // Check data is valid?
	  // frame 0 = frame 3 && frame 12 = 'Q'
	  Delimiter(dataComm_mainBuff, ',', 3, 80, (unsigned char*) COMPID);
	  Delimiter(dataComm_mainBuff, ',', 12, 80, (unsigned char*) ENDBYTE);

	  if(dataComm_mainBuff[0] == COMPID[0] && ENDBYTE[0] == 'Q') {
		  SerialDebug("[MCU] -> Data is valid\r\n");

		  // Delimit data
		  Delimiter(dataComm_mainBuff, ',', 1, 80, (unsigned char*) SENSOR.timeStemp);
		  Delimiter(dataComm_mainBuff, ',', 2, 80, (unsigned char*) SENSOR.dateStamp);
		  Delimiter(dataComm_mainBuff, ',', 4, 80, (unsigned char*) SENSOR.X);
		  Delimiter(dataComm_mainBuff, ',', 5, 80, (unsigned char*) SENSOR.Y);
		  Delimiter(dataComm_mainBuff, ',', 6, 80, (unsigned char*) SENSOR.Z);
		  Delimiter(dataComm_mainBuff, ',', 7, 80, (unsigned char*) SENSOR.Huim);
		  Delimiter(dataComm_mainBuff, ',', 8, 80, (unsigned char*) SENSOR.Temp);
		  Delimiter(dataComm_mainBuff, ',', 9, 80, (unsigned char*) SENSOR.Alc);
		  Delimiter(dataComm_mainBuff, ',', 10, 80, (unsigned char*) SENSOR.Carbon);
		  Delimiter(dataComm_mainBuff, ',', 11, 80, (unsigned char*) SENSOR.AirFlow);

		  // Send data to server
		  	  // code here but not now

		  // Send response Code
		  SendData_RS485((char*) 'P');

	  }else {
		  SerialDebug("[MCU] -> Data is not valid\r\n");
		  SendData_RS485((char*) 'F');
	  }

	  // End data process
	  SerialDebug("[MCU] -> End data process\r\n");
	  intterruptEvent_Flag = 0;
	  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);
	}
	// =====================================================================================
  }
}











// user custom functions

// init startup value at boot
void sysValinit(void) {
	sysCounter.main_ms_counter = 0;
	sysCounter.prev_LTEtimeout = 0;
	sysCounter.prev_ERRORtime = 0;
	sysCounter.rebootCount = 0;

	sysCounter.CMDrespTime = 1000;	// 1sec.

	sysFlag.LTE_CMD_Send = 0;
	sysFlag.LTE_ERROR = 0;
}


// init sensor variable handle
// set/clear buffer array
void sensorValInit(void) {
	SENSOR.dateStamp[0] = '\0';
	SENSOR.dateStamp[0] = '\0';
	SENSOR.X[0] = '\0';
	SENSOR.Y[0] = '\0';
	SENSOR.Z[0] = '\0';
	SENSOR.Huim[0] = '\0';
	SENSOR.Temp[0] = '\0';
	SENSOR.Alc[0] = '\0';
	SENSOR.Carbon[0] = '\0';
	SENSOR.AirFlow[0] = '\0';
}


// Send text via USB VCOM port
void SerialDebug(char *msgDebug) {
	while(CDC_Transmit_FS((uint8_t *) msgDebug, strlen(msgDebug)) != USBD_OK);
}


// RS485 Tx [Polling method]
void SendData_RS485(char *msg) {
	HAL_GPIO_WritePin(GPIOA, RS485_TxMode, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_UART_Transmit(&huart1, (uint8_t *) msg, strlen(msg), 10);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, RS485_TxMode, GPIO_PIN_RESET);
}

void SendCMD_LTE(char *msg) {
	HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 10);
}


// UART1/UART2 Rx used DMA
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	// For LTE module
	if(huart -> Instance == USART2) {
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t *)Rx2Buff, Rx2Buff_Size);

		memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));				// Clear main buffer
		memcpy(lteComm_MainBuff, Rx2Buff, Size);								// Copy char from UART buffer -> main buffer
		memset(Rx2Buff, 0x00, sizeof(Rx2Buff));									// Clear UART buffer
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
	}

	// For communicate with master
	if(huart -> Instance == USART1) {
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)Rx1Buff, Rx1Buff_Size);

		memset(dataComm_mainBuff, 0x00, sizeof(dataComm_mainBuff));				// Clear main buffer
		memcpy(dataComm_mainBuff, Rx1Buff, Size);								// Copy char from UART buffer -> main buffer
		memset(Rx1Buff, 0x00, sizeof(Rx1Buff));									// Clear UART buffer
		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
	}
}


// find target in string
int findTarget(const char *inStr, const char *target) {
    int i, j;
    for (i = 0; inStr[i] != '\0'; i++) {
        j = 0;
        while (target[j] != '\0' && inStr[i + j] == target[j]) {
            j++;
        }
        if (target[j] == '\0') {
            return 1; // Return 1 if found
        }
    }
    return 0; // Return 0 if not found
}


// Timer4 call back
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim -> Instance == TIM4) {
		sysCounter.main_ms_counter++;
	}
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
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 36000-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 2-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

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
  huart1.Init.BaudRate = 9600;
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB5 PB6 PB7
                           PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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

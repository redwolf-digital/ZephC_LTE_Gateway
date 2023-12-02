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




//uint16_t main_ms_counter = 0;


char textBuffer[125];
int debugValue = 0;

//uint8_t Rx1Buff[Rx1Buff_Size];
//uint8_t Rx2Buff[Rx2Buff_Size];
//uint8_t dataComm_mainBuff[dataComm_MainBuff_S];
//uint8_t lteComm_MainBuff[lteComm_MainBuff_S];

char Rx1Buff[Rx1Buff_Size];
char Rx2Buff[Rx2Buff_Size];
char dataComm_mainBuff[dataComm_MainBuff_S];
char lteComm_MainBuff[lteComm_MainBuff_S];



TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_rx;

// custom HandleTypeDef
SysTimer_HandleTypeDef sysCounter;
SysFlag_HandleTypeDef sysFlag;


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM4_Init(void);



int main(void) {

  sysValinit();

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




  /*
   *			MAIN CODE
   * */

  // Wait LTE module boot
BOOTSEQ :

  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_SET);			// BUSY 1
  HAL_GPIO_WritePin(GPIOB, ONLINE, GPIO_PIN_RESET);		// ONLINE 0
  while(sysCounter.main_ms_counter < 500);				// Wait MCU boot
  SerialDebug("[MCU] -> Wait system boot 30sec.\r\n");

  while(sysCounter.main_ms_counter < LTEbootTime);		// Wait LTE module boot
  initLTE();											// Start init LTE module


  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);		// BUSY 0
  SerialDebug("[MCU] -> System init done\r\n");

  while(1) {
	  //code
	  // if LTE ERROR
	  while(sysFlag.LTE_ERROR == 1) {
		  if(sysCounter.main_ms_counter == 0) {
			  sysCounter.prev_ERRORtime = 0;
		  }

		  while((sysCounter.main_ms_counter - sysCounter.prev_ERRORtime) >= 500) {
			  HAL_GPIO_TogglePin(GPIOB, ERROR);
			  sysCounter.prev_ERRORtime = sysCounter.main_ms_counter;
		  }
	  }


	  // Ping every 5 min. for test internet is working

  }
}











// user custom functions

// init startup value at boot
void sysValinit(void) {
	sysCounter.main_ms_counter = 0;
	sysCounter.prev_LTEtimeout = 0;
	sysCounter.prev_ERRORtime = 0;

	sysCounter.CMDrespTime = 1000;	// 1sec.

	sysFlag.LTE_CMD_Send = 0;
	sysFlag.LTE_ERROR = 0;
}


// Send text via USB VCOM port
void SerialDebug(char *msgDebug) {
	while(CDC_Transmit_FS((uint8_t *) msgDebug, strlen(msgDebug)) != USBD_OK);
}


// UART Tx [Polling method]
void SendData(char *msg) {
	HAL_UART_Transmit(&huart1, (uint8_t *) msg, strlen(msg), 10);
}

void SendCMD_LTE(char *msg) {
	HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 10);
}


// UART Rx used DMA
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


// Timer4 call back
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim -> Instance == TIM4) {
		sysCounter.main_ms_counter++;
	}
}


// EC25 functions

// init sequence
void initLTE(void) {
	SerialDebug("[MCU] -> start initialize LTE module\r\n");

	for(unsigned char countSeq = 0; countSeq < 8; countSeq++) {


		switch(countSeq) {
			// Turn off echo
			case 0 :
				sprintf(textBuffer, "ATE0\r\n");
				break;

			// Low -> High on DTR: Change to command mode while remaining the connected call
			case 1 :
				sprintf(textBuffer, "AT&D1\r\n");
				break;

			// Set frequency band
			case 2 :
				sprintf(textBuffer, "AT+QCFG=\"Band\",511,1\r\n");
				break;

			// Disable GNSS
			case 3 :
				sprintf(textBuffer, "AT+QGPSEND\r\n");
				break;

			// Output via debug UART port
			case 4 :
				sprintf(textBuffer, "AT+QGPSCFG=\"outport\",\"uartdebug\"\r\n");
				break;

			// Enable NMEA
			case 5 :
				sprintf(textBuffer, "AT+QGPSCFG=\"nmeasrc\",1\r\n");
				break;

			// NMEA type output GPRMC only
			case 6 :
				sprintf(textBuffer, "AT+QGPSCFG=\"gpsnmeatype\",2\r\n");
				break;

			// // Turn on GNSS mode 1 : Stand-alone
			case 7 :
				sprintf(textBuffer, "AT+QGPS=1\r\n");
				break;

		}



		SendCMD_LTE((char *)textBuffer);						// Send CMD
		sysFlag.LTE_CMD_Send = 1;
		sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;


		while(sysFlag.LTE_CMD_Send == 1) {
			if(findTarget(lteComm_MainBuff, "OK") == 1) {
				SerialDebug("[LTE] -> OK\r\n");

				sysFlag.LTE_CMD_Send = 0;
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
				goto CLEARMAINBUFF;
			}

			else if(findTarget(lteComm_MainBuff, "ERROR") == 1) {
				SerialDebug("[LTE] -> ");
				SerialDebug((char *)lteComm_MainBuff);
				SerialDebug("\r\n");

				sysFlag.LTE_ERROR = 1;
				sysFlag.LTE_CMD_Send = 0;
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
				goto CLEARMAINBUFF;
			}

			else if((sysCounter.main_ms_counter - sysCounter.prev_LTEtimeout) >= sysCounter.CMDrespTime) {
				SerialDebug("[MCU] -> LTE TIME OUT\r\n");

				sysFlag.LTE_ERROR = 1;
				sysFlag.LTE_CMD_Send = 0;
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
				goto CLEARMAINBUFF;
			}


		}

		CLEARMAINBUFF:
			memset(textBuffer, 0x00, sizeof(textBuffer));
			memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));
	}
}




/*
 * @brief find string if match it return
 * @retval 0 - Not found
 * @retval 1 - found
 */
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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB5 PB6 PB7 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
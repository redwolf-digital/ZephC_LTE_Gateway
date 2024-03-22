/*
 * Project		: ZephC LTE Gateway
 * Version		: 1.1
 * Revision		: 1.0
 *
 * Author		: REDWOLF DiGiTAL [C. Worakan]
 */

#include "main.h"
#include <string.h>
#include <stdio.h>

#include "msgProcess.h"
#include "GNSSprocess.h"
#include "EXITinti.h"
#include "LTEdriver.h"
#include "initernet.h"

#define ERROR_TICK	1200

uint16_t TICK_COUNT = 0;
uint8_t ACK_FAIL_COUNT = 0;
uint8_t REBOOT_FLAG = 0;

char textBuffer[128];
char URL_temp[350];
char dump_temp[128];
char dump_samping[128];


char Rx1Buff[Rx1Buff_Size];
char Rx2Buff[Rx2Buff_Size];
char dataComm_mainBuff[dataComm_MainBuff_S];
char lteComm_MainBuff[lteComm_MainBuff_S];

unsigned int URL_len;
unsigned char sendURL_flag = 0;



char ENDBYTE[1];


// internal flag
uint8_t AckInternet_ErrCode = 0;
uint8_t AckInternet_flag = 0;
uint8_t reboot_min_count = 0;



TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;
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
static void MX_USART6_UART_Init(void);
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
  MX_USART6_UART_Init();
  MX_TIM4_Init();


  // TIMER4 START
  HAL_TIM_Base_Start_IT(&htim4);


  // DMA LTE
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t *)Rx2Buff, Rx2Buff_Size);
  //__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

  //DMA commMaster
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)Rx1Buff, Rx1Buff_Size);
  //__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);


  // INTERRUPT
  initEXIT();
  resetWDT();

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
  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_SET);			// BUSY -> 1
  HAL_GPIO_WritePin(GPIOB, ACTIVE, GPIO_PIN_SET);		// ACTIVE indicator -> 1
  while(sysCounter.main_ms_counter < 500);				// Wait MCU boot

  UART6_Debug("[MCU] -> WAIT MODEM BOOT 30 SEC.\r\n");

  resetWDT();
  // Initialize LTE module
  while(sysCounter.main_ms_counter < LTEbootTime) {		// Wait LTE module boot
	  intterruptEvent_Flag = 0;
	  resetWDT();
  }

  // Start init LTE module
  UART6_Debug("[MCU] -> INIT MCU\r\n");
  initLTE();
  resetWDT();
  UART6_Debug("[MCU] -> INIT INTERNET\r\n");
//  AckInternet_ErrCode = AckInternet();
  AckInternet();
  AckInternet_flag = 1;
  resetWDT();

  // Respond after boot finish
  UART6_Debug("[MCU] -> SEND RDY\r\n");
  HAL_GPIO_WritePin(GPIOB, RDY, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, RDY, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);		// BUSY -> 0
  HAL_GPIO_WritePin(GPIOB, ACTIVE, GPIO_PIN_RESET);		// ACTIVE indicator -> 0
  resetWDT();
  UART6_Debug("[MCU] -> BOOT DONE\r\n");



  while(1) {
	  // Clear WDT module routine
	  if(sysCounter.main_ms_counter == 0) {
		  sysCounter.prev_ClearWDT = 0;
	  }
	  while((sysCounter.main_ms_counter - sysCounter.prev_ClearWDT) >= 1000) {
		  resetWDT();
		  sysCounter.prev_ClearWDT = sysCounter.main_ms_counter;
	  }


	  // ErrorHandle
	  //Init error please check system
	  while(sysFlag.LTE_INIT_ERROR == 1) {
		  HAL_GPIO_TogglePin(GPIOB, ERROR);
		  TICK_COUNT++;
		  resetWDT();
		  HAL_Delay(250);

		  // Reboot
		  while(TICK_COUNT >= ERROR_TICK) {
			  UART6_Debug("[MCU] -> SEND SHUTDOWN\r\n");
			  SendCMD_LTE("AT+QPOWD=0\r\n");
			  sysFlag.LTE_CMD_Send = 1;

			  while(sysFlag.LTE_CMD_Send == 1) {

				  if(findTarget(lteComm_MainBuff, "POWERED DOWN") == 1) {
					  UART6_Debug("[MCU] -> GET POWERED DOWN GOOD BYE\r\n");
					  REBOOT_FLAG = 1;
				  }

				  while(REBOOT_FLAG == 1) {
					  for(;;);
				  }
			  }
		  }
	  }

	  // Reboot after active failed 5 time
	  while(ACK_FAIL_COUNT >= 5) {
		  HAL_GPIO_WritePin(GPIOB, ERROR, GPIO_PIN_SET);
		  UART6_Debug("[MCU] -> SEND SHUTDOWN\r\n");
		  SendCMD_LTE("AT+QPOWD=0\r\n");
		  sysFlag.LTE_CMD_Send = 1;

		  // Reboot
		  while(sysFlag.LTE_CMD_Send == 1) {
			  if(findTarget(lteComm_MainBuff, "POWERED DOWN")) {
				  UART6_Debug("[MCU] -> GET POWERED DOWN GOOD BYE\r\n");
				  REBOOT_FLAG = 1;
			  }

			  while(REBOOT_FLAG == 1) {
				  for(;;);
			  }
		  }
	  }


	  // Reboot after 5 min.
	  while(sysFlag.LTE_ERROR == 1) {
		  resetWDT();
		  if(sysCounter.main_ms_counter == 0) {
			  sysCounter.rebootCount = 0;
		  }
		  if((sysCounter.main_ms_counter - sysCounter.rebootCount) >= 60000) {
			  reboot_min_count++;
			  sysCounter.rebootCount = sysCounter.main_ms_counter;
		  }
		  while(reboot_min_count >= 5) {
			  if(SHUTDOWN_LTE() == 1) {
				  HAL_Delay(50);
				  HAL_NVIC_SystemReset();
			  }else {
				  sysFlag.LTE_ERROR = 0;
				  sysFlag.LTE_INIT_ERROR = 1;
				  sysCounter.rebootCount = sysCounter.main_ms_counter;
			  }
		  }

		  if(intterruptEvent_Flag == 1) {
			  SendData_RS485((char*) "X");
			  intterruptEvent_Flag = 0;
		  }
	  }




	  /*
	   * ------------------------------------------------------------------------------------
	   * 									MAIN TASK
	   * ------------------------------------------------------------------------------------
	   */
	  while(intterruptEvent_Flag == 1 && AckInternet_flag == 0) {
		  UART6_Debug("[MCU] -> CHECK INTERNET\r\n");

		  if(Activate() == 0) {
			  UART6_Debug("[MCU] -> CONNECT PASS\r\n");
			  AckInternet_flag = 1;
			  resetWDT();
		  }else {
			  UART6_Debug("[MCU] -> CONNECT FAIL\r\n");
			  ACK_FAIL_COUNT++;
			  SendData_RS485((char*) "X");

			  intterruptEvent_Flag = 0;
			  AckInternet_flag = 0;
			  resetWDT();
		  }
	  }

	  MAINTASK :
	  while(intterruptEvent_Flag == 1 && AckInternet_flag == 1) {
			HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_SET);				    	// BUSY !!!!
			HAL_GPIO_WritePin(GPIOB, ACTIVE, GPIO_PIN_SET);					// ACTIVE indicator -> 1


			UART6_Debug("[MCU] -> GET RTS\r\n");
			resetWDT();

//			UART6_Debug("[MCU] -> CHECK INTERNET CONN.\r\n");
//			// Activate internet connection
//		    if(AckInternet_flag == 0) {
//		    	if(Activate() == 0) {
//		    		UART6_Debug("[MCU] -> CONNECT\r\n");
//		    		AckInternet_flag = 1;
//		    	}else {
//		    		UART6_Debug("[MCU] -> CONNECT ERROR\r\n");
//		    		SendData_RS485((char*) "X");
//		    		AckInternet_flag = 0;
//		    		intterruptEvent_Flag = 0;
//		    		resetWDT();
//		    	}
//		    }else {
//		    	UART6_Debug("[MCU] -> ACTIVE BEFOR\r\n");
//		    }
//		    resetWDT();

//		    while(AckInternet_flag == 1) {
				// Clear buffer
				sensorValInit();
				memset(SENSOR.COMPID, 0x00, sizeof(SENSOR.COMPID));
				memset(ENDBYTE, 0x00, sizeof(ENDBYTE));
				resetWDT();

				HAL_Delay(10);
				// Call GPS
				if(callGNSS(GNSS.lat, GNSS.lon, GNSS.time, GNSS.date) == 1) {
					UART6_Debug("[GPS] -> NMEA CRC PASS\r\n");
				}else {
					UART6_Debug("[GPS] -> NMEA CRC INVALID/FAIL\r\n");
				}


				resetWDT();
				// Send RDY signal
				HAL_Delay(10);
				UART6_Debug("[MCU] -> SEND RDY\r\n");
				HAL_GPIO_WritePin(GPIOB, RDY, GPIO_PIN_SET);
				HAL_Delay(1);
				HAL_GPIO_WritePin(GPIOB, RDY, GPIO_PIN_RESET);
				resetWDT();



				// Wait DMA put data to buffer
				sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
				while(*dataComm_mainBuff == '\0') {
				  // Timeout conditions
				  if(sysCounter.main_ms_counter == 0) {
					  sysCounter.prev_msgTimeOut = 0;
				  }

				  if((sysCounter.main_ms_counter - sysCounter.prev_msgTimeOut) >= 1000) {
					  UART6_Debug("[MCU] -> UART TIME OUT\r\n");
					  SendData_RS485((char*) "F");

					  intterruptEvent_Flag = 0;
					  HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);
					  HAL_GPIO_WritePin(GPIOB, ACTIVE, GPIO_PIN_RESET);
					  sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
					  goto MAINTASK;
				  }
				}
				sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
				resetWDT();


				// Check data is valid?
				// frame 0 = frame 3 && frame 12 = 'Q'
				UART6_Debug("[MCU] -> CHECK DATA FRAME\r\n");

				memset(dump_samping, 0x00, sizeof(dump_samping));
				memcpy(dump_samping, dataComm_mainBuff, sizeof(dump_samping));

				Delimiter(dataComm_mainBuff, ',', 3, 80, (unsigned char*) SENSOR.COMPID);
				Delimiter(dataComm_mainBuff, ',', 12, 80, (unsigned char*) ENDBYTE);

				if(dataComm_mainBuff[0] == SENSOR.COMPID[0] && ENDBYTE[0] == 'Q') {
				  resetWDT();
				  UART6_Debug("[MCU] -> DATA IS VALID\r\n");

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

				  resetWDT();

				  // Send data to server
				  HAL_Delay(1);
				  memset(URL_temp, 0x00, sizeof(URL_temp));
				  URL_len = httpSend(GNSS.lat, GNSS.lon, SENSOR.COMPID, SENSOR.timeStemp, SENSOR.dateStamp, SENSOR.X, SENSOR.Y, SENSOR.Z, SENSOR.Huim, SENSOR.Temp, SENSOR.Alc, SENSOR.Carbon, SENSOR.AirFlow, URL_temp);

				  UART6_Debug("URL DUMP :\n");
				  UART6_Debug((char*) URL_temp);
				  UART6_Debug("\r\n");

				  resetWDT();

				  HAL_Delay(10);

				  memset(textBuffer, 0x00, sizeof(textBuffer));
				  memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));


				  for(unsigned char count = 0; count < 3; count++) {
					  switch(count) {
						  case 0 :
							  sprintf(textBuffer, "AT+QHTTPURL=%d,80\r\n", URL_len);
							  break;

						  case 1 :
							  sprintf(textBuffer, "AT+QHTTPPOST=1,60,60\r\n");
							  break;

						  case 2 :
							  HAL_Delay(3000);
							  sprintf(textBuffer, "AT+QHTTPREAD=80\r\n");
							  break;
					  }


					  UART6_Debug((char *) textBuffer);
					  UART6_Debug("\r\n");
					  SendCMD_LTE((char *) textBuffer);
					  sendURL_flag = 1;

					  while(sendURL_flag == 1) {
						  if(findTarget(lteComm_MainBuff, "CONNECT") == 1) {
							  if(count == 0) {
								  SendCMD_LTE((char *) URL_temp);
							  }

							  if(count == 1) {
								  SendCMD_LTE((char *) "\r");
							  }

							  if(count == 2) {
								  memset(dump_temp, 0x00, sizeof(dump_temp));
								  memcpy(dump_temp, lteComm_MainBuff, sizeof(dump_temp));

								  UART6_Debug("\n");
								  UART6_Debug((char *) dump_temp);
								  UART6_Debug("\r\n");

								  SendData_RS485((char*) "P");
								  sendURL_flag = 0;
							  }

							  memset(textBuffer, 0x00, sizeof(textBuffer));
							  memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));
						  }

						  else if(findTarget(lteComm_MainBuff, "OK") == 1) {

							  UART6_Debug("[LTE] -> OK\r\n");

							  memset(textBuffer, 0x00, sizeof(textBuffer));
							  memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));

							  sendURL_flag = 0;
						  }


						  else if(findTarget(lteComm_MainBuff, "ERROR") == 1) {

							  UART6_Debug("[LTE] -> ERROR\r\n");

							  SendData_RS485((char*) "X");


							  memset(textBuffer, 0x00, sizeof(textBuffer));
							  memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));

							  sysFlag.LTE_ERROR = 1;
							  sendURL_flag = 0;
							  intterruptEvent_Flag = 0;
						  }
					  }
				  }
				}else {
					UART6_Debug("[MCU] -> DATA NOT VALID\r\n");
					SendData_RS485((char*) "F");
				}

				resetWDT();

				//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				UART6_Debug("[MCU] -> DEACTIVATE INTERNET\r\n");
				Deactivate();
				AckInternet_flag = 0;


				UART6_Debug("[MCU] -> END PROCESS\r\n\r\n");

				intterruptEvent_Flag = 0;
				HAL_GPIO_WritePin(GPIOB, BUSY, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, ACTIVE, GPIO_PIN_RESET);
//	  	  	  }
		}
		// =====================================================================================


	} // END MAIN LOOP
} // END MAIN











// user custom functions


// init startup value at boot
void sysValinit(void) {
	sysCounter.main_ms_counter = 0;
	sysCounter.prev_LTEtimeout = 0;
	sysCounter.prev_ERRORtime = 0;
	sysCounter.rebootCount = 0;
	sysCounter.prev_ClearWDT = 0;

	sysCounter.CMDrespTime = 1000;	// 1sec.

	sysFlag.LTE_CMD_Send = 0;
	sysFlag.LTE_INIT_ERROR = 0;
	sysFlag.LTE_ERROR = 0;
}


// init sensor variable handle
// set/clear buffer array
void sensorValInit(void) {
	memset(SENSOR.COMPID, 0x00, sizeof(SENSOR.COMPID));
	memset(SENSOR.dateStamp, 0x00, sizeof(SENSOR.dateStamp));
	memset(SENSOR.timeStemp, 0x00, sizeof(SENSOR.timeStemp));
	memset(SENSOR.X, 0x00, sizeof(SENSOR.X));
	memset(SENSOR.Y, 0x00, sizeof(SENSOR.Y));
	memset(SENSOR.Z, 0x00, sizeof(SENSOR.Z));
	memset(SENSOR.Huim, 0x00, sizeof(SENSOR.Huim));
	memset(SENSOR.Temp, 0x00, sizeof(SENSOR.Temp));
	memset(SENSOR.Alc, 0x00, sizeof(SENSOR.Alc));
	memset(SENSOR.Carbon, 0x00, sizeof(SENSOR.Carbon));
	memset(SENSOR.AirFlow, 0x00, sizeof(SENSOR.AirFlow));
}


// UART Debug
void UART6_Debug(char* msg) {
	HAL_UART_Transmit(&huart6, (uint8_t *) msg, strlen(msg), 1000);
}

// RS485 Tx [Polling method]
void SendData_RS485(char *msg) {
	HAL_UART_Transmit(&huart1, (uint8_t *) msg, strlen(msg), 1000);
}

void SendCMD_LTE(char *msg) {
	HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 1000);
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


// Clear communication buffer
void Clear_Buff_Commu(void) {
	memset(dataComm_mainBuff, 0x00, sizeof(dataComm_mainBuff));
}


void resetWDT(void) {
	HAL_GPIO_WritePin(GPIOB, RESET_WDT, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, RESET_WDT, GPIO_PIN_RESET);
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
  huart6.Init.BaudRate = 115200;
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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB4 PB5 PB6
                           PB7 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_9;
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

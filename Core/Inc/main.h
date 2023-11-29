#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"


#define Rx1Buff_Size			255
#define Rx2Buff_Size			255
#define dataComm_MainBuff_S		255
#define lteComm_MainBuff_S		255

#define LTEbootTime				30000

//GPIO
#define BUSY 					GPIO_PIN_5
#define ONLINE					GPIO_PIN_6
#define RDY						GPIO_PIN_7
#define RTS						GPIO_PIN_8
#define ERROR					GPIO_PIN_9


typedef struct{
	uint16_t main_ms_counter;
	uint16_t prev_LTEtimeout;
	uint16_t prev_ERRORtime;

	uint16_t CMDrespTime;
}SysTimer_HandleTypeDef;

typedef struct{
	unsigned char LTE_CMD_Send;
	unsigned char LTE_ERROR;
}SysFlag_HandleTypeDef;


void SerialDebug(char *msgDebug);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void SendData(char *msg);
void SendCMD_LTE(char *msg);

void sysValinit(void);

int LTErespondOK(char* inStr, uint16_t len);
void initLTE(void);


void Error_Handler(void);



#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

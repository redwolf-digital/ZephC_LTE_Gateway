#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"


#define Rx1Buff_Size			128
#define Rx2Buff_Size			255

#define dataComm_MainBuff_S		128
#define lteComm_MainBuff_S		255

#define LTEbootTime				30000
#define msgTimeOut				5000

//GPIO
#define BUSY 					GPIO_PIN_5
#define ONLINE					GPIO_PIN_6
#define RDY						GPIO_PIN_7
#define ERROR					GPIO_PIN_9

#define RS485_TxMode			GPIO_PIN_8




typedef struct {
	uint16_t main_ms_counter;
	uint16_t prev_LTEtimeout;
	uint16_t prev_ERRORtime;
	uint16_t prev_msgTimeOut;

	uint8_t rebootCount;

	uint16_t CMDrespTime;
}SysTimer_HandleTypeDef;


typedef struct {
	unsigned char LTE_CMD_Send;
	unsigned char LTE_ERROR;
}SysFlag_HandleTypeDef;


typedef struct {
	char lat[10];
	char lon[10];
}GNSS_HandleTypeDef;


typedef struct {
	char dateStamp[8];
	char timeStemp[8];
	char ID[1];
	char X[6];
	char Y[6];
	char Z[6];
	char Huim[6];
	char Temp[6];
	char Alc[6];
	char Carbon[6];
	char AirFlow[6];
}Sensor_HandleTypeDef;







extern SysTimer_HandleTypeDef sysCounter;
extern SysFlag_HandleTypeDef sysFlag;
extern GNSS_HandleTypeDef GNSS;
extern Sensor_HandleTypeDef SENSOR;


extern char Rx1Buff[Rx1Buff_Size];
extern char Rx2Buff[Rx2Buff_Size];
extern char dataComm_mainBuff[dataComm_MainBuff_S];
extern char lteComm_MainBuff[lteComm_MainBuff_S];


extern void SendData_RS485(char *msg);
extern void SendCMD_LTE(char *msg);
extern void SerialDebug(char *msgDebug);


extern int findTarget(const char *inStr, const char *target);


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);

void sysValinit(void);
void sensorValInit(void);







void Error_Handler(void);



#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

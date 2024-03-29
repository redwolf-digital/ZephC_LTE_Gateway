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
#define ACTIVE					GPIO_PIN_6
#define RDY						GPIO_PIN_7
#define ERROR					GPIO_PIN_9
#define	RESET_WDT				GPIO_PIN_4

#define RS485_TxMode			GPIO_PIN_8



typedef struct {
//	uint16_t main_ms_counter;
//	uint16_t prev_LTEtimeout;
//	uint16_t prev_ERRORtime;
//	uint16_t prev_msgTimeOut;
//	uint16_t prev_ClearWDT;
	uint32_t main_ms_counter;
	uint32_t prev_LTEtimeout;
	uint32_t prev_ERRORtime;
	uint32_t prev_msgTimeOut;
	uint32_t prev_ClearWDT;
	uint32_t prev_GPSupdate;

	uint8_t rebootCount;

	uint16_t CMDrespTime;
}SysTimer_HandleTypeDef;


typedef struct {
	unsigned char LTE_CMD_Send;
	unsigned char LTE_INIT_ERROR;
	unsigned char LTE_ERROR;
	unsigned char UPDATE_GPS;
}SysFlag_HandleTypeDef;


typedef struct {
	char lat[16];
	char lon[16];
	char time[16];
	char date[16];
}GNSS_HandleTypeDef;


typedef struct {
	char COMPID[4];
	char dateStamp[16];
	char timeStemp[16];
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
void UART6_Debug(char* msg);


extern int findTarget(const char *inStr, const char *target);


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);

void sysValinit(void);
void sensorValInit(void);
void Clear_Buff_Commu(void);
void resetWDT(void);






void Error_Handler(void);



#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

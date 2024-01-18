/*
 * initernet.c
 *
 *  Created on: Jan 8, 2024
 *      Author: REDWOLF DiGiTAL
 */

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "LTEdriver.h"

char TextNetTemp[125];



void clearText_net_Temp(void) {
	memset(TextNetTemp, 0x00, sizeof(TextNetTemp));
}




unsigned char initInternet(void) {

	SerialDebug("[MCU] -> start initialize LTE module\r\n");

	for(unsigned char countSeq = 0; countSeq < 5; countSeq++) {
		switch(countSeq) {
			case 0 :
				sprintf(TextNetTemp, "AT+QICSGP=1,1,\"INTERNET\",\"\",\"\",1\r\n");
				break;

			case 1 :
				sprintf(TextNetTemp, "AT+QIACT=1\r\n");
				break;

			case 2 :
				sprintf(TextNetTemp, "AT+QIACT?\r\n");
				break;

			case 3 :
				sprintf(TextNetTemp, "AT+QIDNSCFG=1,\"8.8.8.8\",\"1.1.1.1\"\r\n");
				break;
		}

		SendCMD_LTE((char *)TextNetTemp);	// Sned CMD
		sysFlag.LTE_CMD_Send = 1;
		sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;


		while(sysFlag.LTE_CMD_Send == 1) {

			// Reset counter
			if(sysCounter.main_ms_counter == 0) {
				sysCounter.prev_LTEtimeout = 0;
			}

			// OK conditions
			if(findTarget(lteComm_MainBuff, "OK") == 1) {
				SerialDebug("[LTE] -> OK\r\n");

				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_net_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}

			// Error conditions
			else if(findTarget(lteComm_MainBuff, "ERROR") == 1) {
				SerialDebug("[LTE] -> ");
				SerialDebug((char *)lteComm_MainBuff);

				sysFlag.LTE_ERROR = 1;

				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_net_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}

			// Timeout Conditions
			else if((sysCounter.main_ms_counter - sysCounter.prev_LTEtimeout) >= sysCounter.CMDrespTime) {
				SerialDebug("[MCU] -> LTE TIME OUT\r\n");
				sysFlag.LTE_ERROR = 1;
				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_net_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}
		}
	}
}

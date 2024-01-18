/*
 * GNSSprocess.c
 *
 *  Created on: Dec 18, 2023
 *      Author: REDWOLF DiGiTAL
 */

#include "main.h"
#include "GNSSprocess.h"
#include "msgProcess.h"


char* status;
unsigned char latTemp[16];
unsigned char lonTemp[16];

unsigned char processFlag = 0;
unsigned char returnValue;


// Get GPS data
// return 1 if process is done
// return 2 if NMEA CRC is fail
unsigned char callGNSS(void) {
	SendCMD_LTE("AT+QGPSCFG=\"outport\",\"uartdebug\"\r\n");

	while(findTarget(lteComm_MainBuff, "OK") != 0);
	HAL_Delay(2);

	processFlag = NMEACRCCal((unsigned char *) lteComm_MainBuff);

	if(processFlag == 1) {
		Delimiter(lteComm_MainBuff, ',', 3, 80, latTemp);
		Delimiter(lteComm_MainBuff, ',', 5, 80, lonTemp);
		returnValue = 1;
	}else {
		returnValue = 2;
	}

	SendCMD_LTE("AT+QGPSCFG=\"outport\",\"none\"\r\n");
	while(findTarget(lteComm_MainBuff, "OK") != 0);
	return returnValue;
}

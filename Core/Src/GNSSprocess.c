/*
 * GNSSprocess.c
 *
 *  Created on: Dec 18, 2023
 *      Author: REDWOLF DiGiTAL
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "GNSSprocess.h"
#include "msgProcess.h"


char* status;

char GNSS_temp[128];
unsigned char latTemp[16];
unsigned char lonTemp[16];

unsigned char processFlag = 0;
unsigned char returnValue;


// Get GPS data
// return 1 if process is done
// return 2 if NMEA CRC is fail
unsigned char callGNSS(void) {
	memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));
	SendCMD_LTE((char *) "AT+QGPSCFG=\"outport\",\"uartdebug\"\r\n");

	while(findTarget(lteComm_MainBuff, "GPRMC") != 1);
	memcpy(GNSS_temp, lteComm_MainBuff, sizeof(GNSS_temp));
	HAL_Delay(2);

	processFlag = NMEACRCCal((unsigned char *) GNSS_temp);

	if(processFlag == 1) {
		Delimiter(GNSS_temp, ',', 3, 80, latTemp);
		Delimiter(GNSS_temp, ',', 5, 80, lonTemp);

		sprintf((char *)latTemp, "%d.%d",atoi((char *)latTemp)/100, atoi((char *)latTemp)%100);
		sprintf((char *)lonTemp, "%d.%d",atoi((char *)lonTemp)/100, atoi((char *)lonTemp)%100);

		returnValue = 1;
	}else {
		returnValue = 2;
	}

	SendCMD_LTE((char *) "AT+QGPSCFG=\"outport\",\"none\"\r\n");
	while(findTarget(lteComm_MainBuff, "OK") != 1);

	return returnValue;
}

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
unsigned char callGNSS(char* lat_out, char* lon_out) {
	UART6_Debug("[GPS] -> CALL\r\n");

	memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));
	SendCMD_LTE((char *) "AT+QGPSCFG=\"outport\",\"uartdebug\"\r\n");
	UART6_Debug("[GPS] -> REQ.\r\n");


	// TIME OUT
	sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
	while(findTarget(lteComm_MainBuff, "GPRMC") != 1){
		if(sysCounter.main_ms_counter == 0) {
			sysCounter.prev_msgTimeOut = 0;
		}

		if((sysCounter.main_ms_counter - sysCounter.prev_msgTimeOut) >= 10000) {
			UART6_Debug("[GPS] -> TIMEOUT\r\n");
			returnValue = 2;
			sysCounter.prev_msgTimeOut = sysCounter.main_ms_counter;
			goto END;
		}
	}

	memcpy(GNSS_temp, lteComm_MainBuff, sizeof(GNSS_temp));
	HAL_Delay(2);

	processFlag = NMEACRCCal((unsigned char *) GNSS_temp);

	if(processFlag == 1) {
		UART6_Debug("[GPS] -> PROCESS\r\n");
		// Clear old pos.
		memset(lat_out, 0x00, strlen(lat_out));
		memset(lon_out, 0x00, strlen(lon_out));
		// Delimit
		Delimiter(GNSS_temp, ',', 3, 80, latTemp);
		Delimiter(GNSS_temp, ',', 5, 80, lonTemp);
		// Put new pos.
		NMEAdecoder((char *)latTemp, lat_out, 0);
		NMEAdecoder((char *)lonTemp, lon_out, 1);

		returnValue = 1;
	}else {
		returnValue = 2;
	}

	// Dumb way to turn off GMSS @_@
	SendCMD_LTE((char *) "AT+QGPSCFG=\"outport\",\"none\"\r\n");
	while(findTarget(lteComm_MainBuff, "OK") != 1);



END:
	UART6_Debug("[GPS] -> DONE\r\n");
	return returnValue;
}


void NMEAdecoder(char* input, char* output, unsigned char mode) {
    char temp[sizeof(input)+2];

    for(int index = 0; index < sizeof(input)+2; index++) {
        *(temp+index) = *(input+index);
    }

    if(mode == 0) { // LAT
        sprintf(output, "%.6f",(atoi(temp)/100)+(atof(temp+2)/60));
    }else { // LON
        sprintf(output, "%.6f",(atoi(temp)/1000)+(atof(temp+3)/60));
    }

}


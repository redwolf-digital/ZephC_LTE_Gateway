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
char latDir[2];
char lonDir[2];

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
	UART6_Debug("[GPS] -> DUMP\r\n");
	UART6_Debug(GNSS_temp);
	UART6_Debug("\r\n");
	HAL_Delay(2);

	processFlag = NMEACRCCal((unsigned char *) GNSS_temp);

	if(processFlag == 1) {
		UART6_Debug("[GPS] -> PROCESS\r\n");
		// Clear old pos.
		memset(lat_out, 0x00, strlen(lat_out));
		memset(lon_out, 0x00, strlen(lon_out));
		memset(latDir, 0x00, strlen(latDir));
		memset(lonDir, 0x00, strlen(lonDir));

		// Delimit
		Delimiter(GNSS_temp, ',', 3, 80, latTemp);
		Delimiter(GNSS_temp, ',', 4, 80, (unsigned char *)latDir);
		Delimiter(GNSS_temp, ',', 5, 80, lonTemp);
		Delimiter(GNSS_temp, ',', 6, 80, (unsigned char *)lonDir);

		// Put new pos.
		NMEAdecoder((char *)latTemp, &latDir[0], lat_out);
		NMEAdecoder((char *)lonTemp, &lonDir[0], lon_out);

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


void NMEAdecoder(char* NMEAin_C, char* dir, char* out) {
	double conv_f = atof(NMEAin_C);

	int pos_D = (conv_f/100);
	double pos_M = (conv_f - (pos_D*100));
	double final_pos = pos_D + (pos_M/60);

	if(*dir == 'S' || *dir == 'W') {
		final_pos = final_pos*-1;
	}

	sprintf(out, "%.6f", final_pos);

}


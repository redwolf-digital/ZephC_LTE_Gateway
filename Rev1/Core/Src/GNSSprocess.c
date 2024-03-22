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

#define UTC 700


int hr = 0;
int min = 0;
int sec = 0;
unsigned int day = 0;
unsigned int mon = 0;
unsigned int year = 0;
unsigned int daychange = 0;


char* status;

char GNSS_temp[128];
unsigned char latTemp[16];
unsigned char lonTemp[16];
unsigned char timeTemp[16];
unsigned char dateTemp[16];

char latDir[2];
char lonDir[2];

unsigned char processFlag = 0;
unsigned char returnValue;


// Get GPS data
// return 1 if process is done
// return 2 if NMEA CRC is fail
/*
 *  example NMEA data : $GPRMC,161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10
 * 	0	- RMC Protocol header
 * 	1	- UTC time in format hhmmss.sss
 * 	2	- Status A = data valid V = data not valid
 * 	3	- Latitude
 * 	4	- N = north , S = south
 * 	5	- Longitude
 * 	6	- E = east , W = west
 * 	7	- Speed over ground (knots)
 * 	8	- Course over Ground (degrees)
 * 	9	- Date in format ddmmyy
 * 	10	- Magnetic Variations E = east , W = west
 * 	11	- East/West indicator
 * 	12	- Mode
 * 	13	- Checksum
 *
 */
unsigned char callGNSS(char* lat_out, char* lon_out, char* timeS_out, char* dateS_out) {
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
		memset(timeS_out, 0x00, strlen(timeS_out));
		memset(dateS_out, 0x00, strlen(dateS_out));
		memset(latDir, 0x00, strlen(latDir));
		memset(lonDir, 0x00, strlen(lonDir));

		// Delimit
		Delimiter(GNSS_temp, ',', 3, 80, latTemp);
		Delimiter(GNSS_temp, ',', 4, 80, (unsigned char *)latDir);
		Delimiter(GNSS_temp, ',', 5, 80, lonTemp);
		Delimiter(GNSS_temp, ',', 6, 80, (unsigned char *)lonDir);

		Delimiter(GNSS_temp, ',', 1, 80, timeTemp);
		Delimiter(GNSS_temp, ',', 9, 80, dateTemp);

		// Put new pos.
		NMEAdecoder((char *)latTemp, &latDir[0], lat_out);
		NMEAdecoder((char *)lonTemp, &lonDir[0], lon_out);
		// Decode time and date
		NMEAgetTime((char *)timeTemp, timeS_out);
		NMEAgetDate((char *)dateTemp, dateS_out);

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


void NMEAgetTime(char* time_in, char* out) {
	int temp = atoi(time_in);

	hr = (temp/10000) + (UTC/100);
	min = (temp/100)%100 + (UTC%100);
	sec = temp%100;

	if(min > 59) {
		min = min - 60;
		hr++;
	}
	if(hr < 0) {
		hr = 24 + hr;
		daychange--;
	}
	if(hr >= 24) {
		hr = hr - 24;
		daychange++;
	}

	sprintf(out, "%02d:%02d:%02d.000", hr, min, sec);

}


void NMEAgetDate(char* date_in, char* out) {
	int temp = atoi(date_in);

	day = temp/10000;
	mon = (temp/100)%100;
	year = temp%100;

	day = day+daychange;

	sprintf(out, "20%d-%02d-%02d", year, mon, day);

}


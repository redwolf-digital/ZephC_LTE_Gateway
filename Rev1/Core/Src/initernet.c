/*
 * initernet.c
 *
 *  Created on: Jan 8, 2024
 *      Author: REDWOLF DiGiTAL
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "LTEdriver.h"
#include "msgProcess.h"

#define TOKEN "77690b2d-e66e-454a-9760-b644e40cc7fb"

int temp;
char time_temp[12];
char date_temp[12];

char DD[8];
char MM[8];
char YY[8];

char hh[8];
char mm[8];
char ss[8];


char TextNetTemp[125];
unsigned char Error = 0;	// 0 = No error | > 1 Error

char HTTP_URL_Temp[350];	// URL length max 350 byte


void clearText_net_Temp(void) {
	memset(TextNetTemp, 0x00, sizeof(TextNetTemp));
}


// Fix single digit to 2 digit
void addZero(char* in, char* out) {
	temp = atoi(in);

    if(temp < 10){
        sprintf(out, "0%d", temp);
    }else{
        out = in;
    }
}


unsigned char AckInternet(void) {
	Error = 0;

	for(unsigned char countSeq = 0; countSeq < 4; countSeq++) {
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

		UART6_Debug((char *) TextNetTemp);
		UART6_Debug("\r\n");

		SendCMD_LTE((char *) TextNetTemp);
		sysFlag.LTE_CMD_Send = 1;
		sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;


		while(sysFlag.LTE_CMD_Send == 1) {

			// Reset counter
			if(sysCounter.main_ms_counter == 0) {
				sysCounter.prev_LTEtimeout = 0;
			}

			// OK conditions
			if(findTarget(lteComm_MainBuff, "OK") == 1) {
				UART6_Debug("[LTE] -> OK\r\n");

				if(countSeq == 2) {
					UART6_Debug("[LTE] -> DUMP : \n");
					UART6_Debug((char *)lteComm_MainBuff);
					UART6_Debug("\r\n");

					if(findTarget(lteComm_MainBuff, "+QIACT: 1") == 1) {
						Error = 0;
					}
				}

				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_net_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}

			// Error conditions
			else if(findTarget(lteComm_MainBuff, "ERROR") == 1) {
				UART6_Debug("[LTE] -> ERROR\n");
				UART6_Debug((char *)lteComm_MainBuff);
				UART6_Debug("\r\n");



				Error++;

				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_net_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}

			// Timeout Conditions
			else if((sysCounter.main_ms_counter - sysCounter.prev_LTEtimeout) >= sysCounter.CMDrespTime) {
				if(sysCounter.main_ms_counter == 0) {
					sysCounter.prev_LTEtimeout = 0;
				}

				UART6_Debug("[MCU] -> LTE TIME OUT\n");

				Error++;
				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_net_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}
		}
	}

	return Error;
}


//send data to server
unsigned int httpSend(char* lat, char* lon, char* device_ID, char* time_s, char* date_s, char* x, char* y, char* z, char* humi, char* temp, char* eth, char* carbon, char* airflow, char* out) {
	memset(HTTP_URL_Temp, 0x00, sizeof(HTTP_URL_Temp));
	memset(time_temp, 0x00, sizeof(time_temp));
	memset(date_temp, 0x00, sizeof(date_temp));
	memset(hh, 0x00, sizeof(hh));
	memset(mm, 0x00, sizeof(mm));
	memset(ss, 0x00, sizeof(ss));
	memset(YY, 0x00, sizeof(YY));
	memset(MM, 0x00, sizeof(MM));
	memset(DD, 0x00, sizeof(DD));

	// Delimiter hot fix WuW
	sprintf(time_temp, "T:%s", time_s);
	sprintf(date_temp, "D/%s", date_s);


	// Delimit
	Delimiter(time_temp, ':', 1, 80, (unsigned char*) hh);
	Delimiter(time_temp, ':', 2, 80, (unsigned char*) mm);
	Delimiter(time_temp, ':', 3, 80, (unsigned char*) ss);

	Delimiter(date_temp, '/', 1, 80, (unsigned char*) DD);
	Delimiter(date_temp, '/', 2, 80, (unsigned char*) MM);
	Delimiter(date_temp, '/', 3, 80, (unsigned char*) YY);
	// Check date and time to 2 digit
	addZero(hh, hh);
	addZero(mm, mm);
	addZero(ss, ss);
	addZero(YY, YY);
	addZero(MM, MM);
	addZero(DD, DD);

	sprintf(HTTP_URL_Temp, "http://rtls.lailab.online/api/ingest_sensor_data?token=%s&device_id=%s&time=%s&date=%s&device_name=%s&x=%s&y=%s&z=%s&humidity=%s&temp=%s&etha=%s&co2=%s&airflow=%s&symbol=Q&date_now=20%s-%s-%sT%s:%s:%s.000Z&lat=%s&lon=%s", TOKEN, device_ID, time_s, date_s, device_ID, x, y, z, humi, temp, eth, carbon, airflow, YY, MM, DD, hh, mm, ss, lat, lon);

	memcpy(out, HTTP_URL_Temp, sizeof(HTTP_URL_Temp));

	return strlen(HTTP_URL_Temp);
}


// Deactivate HTTP/TCP-IP context
// Return 0 -> PASS
// Return 1 -> ERROR
// Return 2 -> TIME OUT
unsigned char Activate(void) {
	sysFlag.LTE_CMD_Send = 0;
	Error = 0;

	UART6_Debug("[INTERNET] -> ACTIVATE INTERNET\n");

	SendCMD_LTE("AT+QIACT=1\r\n");
	sysFlag.LTE_CMD_Send = 1;

	sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;

	while(sysFlag.LTE_CMD_Send == 1) {
		// PASS
		if(findTarget(lteComm_MainBuff, "OK") == 1) {
			UART6_Debug("[INTERNET] -> DONE\n");
			Error = 0;
			sysFlag.LTE_CMD_Send = 0;
		}
		// ERROR
		if(findTarget(lteComm_MainBuff, "ERROR") == 1) {
			UART6_Debug("[INTERNET] -> ERROR\n");
			Error = 1;
			sysFlag.LTE_CMD_Send = 0;
		}

		// Timeout Conditions
		if((sysCounter.main_ms_counter - sysCounter.prev_LTEtimeout) >= 20000) {
			if(sysCounter.main_ms_counter == 0) {
				sysCounter.prev_msgTimeOut = 0;
			}

			Error = 2;
			UART6_Debug("[INTERNET] -> TIME OUT\n");
			sysFlag.LTE_CMD_Send = 0;
			sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
		}

	}

	return Error;
}


// Deactivate HTTP/TCP-IP context
// Return 0 -> PASS
// Return 1 -> ERROR
// Return 2 -> TIME OUT
unsigned char Deactivate(void) {
	sysFlag.LTE_CMD_Send = 0;
	Error = 0;

	UART6_Debug("[INTERNET] -> DEACTIVATE INTERNET\n");

	SendCMD_LTE("AT+QIDEACT=1\r\n");
	sysFlag.LTE_CMD_Send = 1;

	sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;

	while(sysFlag.LTE_CMD_Send == 1) {
		// PASS
		if(findTarget(lteComm_MainBuff, "OK") == 1) {
			UART6_Debug("[INTERNET] -> DONE\n");
			Error = 0;
			sysFlag.LTE_CMD_Send = 0;
		}
		// ERROR
		if(findTarget(lteComm_MainBuff, "ERROR") == 1) {
			UART6_Debug("[INTERNET] -> ERROR\n");
			Error = 1;
			sysFlag.LTE_CMD_Send = 0;
		}

		// Timeout Conditions
		if((sysCounter.main_ms_counter - sysCounter.prev_LTEtimeout) >= 30000) {
			if(sysCounter.main_ms_counter == 0) {
				sysCounter.prev_msgTimeOut = 0;
			}

			Error = 2;
			UART6_Debug("[INTERNET] -> TIME OUT\n");
			sysFlag.LTE_CMD_Send = 0;
			sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
		}

	}

	return Error;
}



// Ping to server
// Return 2 -> not ready
// Return 1 -> ready
unsigned char ping(void) {
	SendCMD_LTE("AT+QPING=1,\"http://rtls.lailab.online\",60,1\r\n");
	while(findTarget(lteComm_MainBuff, "+QPING") != 1);
	if(lteComm_MainBuff[8] == 0) {
		return 1;
	}else {
		return 2;
	}
}

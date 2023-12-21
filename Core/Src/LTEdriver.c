/*
 * LTEinit.c
 *
 *  Created on: Dec 19, 2023
 *      Author: REDWOLF DiGiTAL
 */

#include <stdio.h>
#include <string.h>
#include "main.h"


char TextTemp[125];
unsigned char globalSnedFlag = 0;





void clearText_Temp(void) {
	memset(TextTemp, 0x00, sizeof(TextTemp));
}

void clearLTE_Temp(void) {
	memset(lteComm_MainBuff, 0x00, sizeof(lteComm_MainBuff));
}






void initLTE(void) {
	SerialDebug("[MCU] -> start initialize LTE module\r\n");

	for(unsigned char countSeq = 0; countSeq < 8; countSeq++) {

		switch(countSeq) {
			case 0 :	// Turn off echo
				sprintf(TextTemp, "ATE0\r\n");
				break;

			case 1 :	// Low -> High on DTR: Change to command mode while remaining the connected call
				sprintf(TextTemp, "AT&D1\r\n");
				break;

			case 2 :	// Set frequency band
				sprintf(TextTemp, "AT+QCFG=\"Band\",511,1\r\n");
				break;

			case 3 :	// Disable GNSS
				sprintf(TextTemp, "AT+QGPSEND\r\n");
				break;

			case 4 :	// Output via debug UART port
				sprintf(TextTemp, "AT+QGPSCFG=\"outport\",\"uartdebug\"\r\n");
				break;

			case 5 :	// Enable NMEA
				sprintf(TextTemp, "AT+QGPSCFG=\"nmeasrc\",1\r\n");
				break;

			case 6 :	// NMEA type output GPRMC only
				sprintf(TextTemp, "AT+QGPSCFG=\"gpsnmeatype\",2\r\n");
				break;

			case 7 :	// Set status network registration
				sprintf(TextTemp, "AT+CREG=1\r\n");
				break;
		}

		SendCMD_LTE((char *)TextTemp);	// Sned CMD
		sysFlag.LTE_CMD_Send = 1;
		sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;

		while(sysFlag.LTE_CMD_Send == 1) {
			// OK conditions
			if(findTarget(lteComm_MainBuff, "OK") == 1) {
				SerialDebug("[LTE] -> OK\r\n");

				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}

			// Error conditions
			else if(findTarget(lteComm_MainBuff, "ERROR") == 1) {
				SerialDebug("[LTE] -> ");
				SerialDebug((char *)lteComm_MainBuff);

				// Case 3 : Disable GNSS fail -> ignore error 505
				if(countSeq == 3 && findTarget(lteComm_MainBuff, "505") == 1) {
					sysFlag.LTE_ERROR = 0;
				}else {
					sysFlag.LTE_ERROR = 1;
				}

				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}

			// Timeout Conditions
			else if((sysCounter.main_ms_counter - sysCounter.prev_LTEtimeout) >= sysCounter.CMDrespTime) {
				SerialDebug("[MCU] -> LTE TIME OUT\r\n");
				sysFlag.LTE_ERROR = 1;
				sysFlag.LTE_CMD_Send = 0;
				clearLTE_Temp();
				clearText_Temp();
				sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;
			}
		}
	}
}


// Network register status
unsigned char networkRegStatus(void) {
	SendCMD_LTE("AT+CREG?\r\n");

	if(findTarget(lteComm_MainBuff, "+CREG: 1") == 1) {
		clearLTE_Temp();
		return 1;
	}else {
		clearLTE_Temp();
		return 0;
	}
}


// Shutdown
/*
 * Return value
 * 			0 - wait
 * 			1 - done
 * 			2 - error/time out
 */
unsigned char SHUTDOWN_LTE(void) {
	SendCMD_LTE("AT+QPOWD\r\n");
	globalSnedFlag = 1;
	sysCounter.prev_LTEtimeout = sysCounter.main_ms_counter;

	// reset counter
	if(sysCounter.main_ms_counter == 0) {
		sysCounter.prev_LTEtimeout = 0;
	}

	while(globalSnedFlag == 1) {
		while(findTarget(lteComm_MainBuff, "POWERED DOWN") == 1) {
			globalSnedFlag = 0;
			return 1;
		}
		// Time out
		if((sysCounter.main_ms_counter - sysCounter.prev_ERRORtime) >= 5000) {
			globalSnedFlag = 0;
			return 2;
		}
	}

	return 0;
}

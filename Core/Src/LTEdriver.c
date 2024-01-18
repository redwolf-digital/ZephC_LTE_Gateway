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

	for(unsigned char countSeq = 0; countSeq < 7; countSeq++) {

		switch(countSeq) {
			case 0 :	// Turn off echo
				sprintf(TextTemp, "ATE0\r\n");
				break;

			case 1 :
				sprintf(TextTemp, "AT&D1\r\n");
				break;

			case 2 :
				sprintf(TextTemp, "AT+QGPSEND\r\n");
				break;

			case 3 :
				sprintf(TextTemp, "AT+QGPSCFG=\"outport\",\"none\"\r\n");
				break;

			case 4 :
				sprintf(TextTemp, "AT+QGPSCFG=\"nmeasrc\",1\r\n");
				break;

			case 5 :
				sprintf(TextTemp, "AT+QGPSCFG=\"gpsnmeatype\",2\r\n");
				break;

			case 6 :
				sprintf(TextTemp, "AT+QGPS\r\n");
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
				if(countSeq == 2 && findTarget(lteComm_MainBuff, "505") == 1) {
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

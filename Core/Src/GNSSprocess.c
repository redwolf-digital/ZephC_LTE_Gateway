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
unsigned char latTemp[10];
unsigned char lonTemp[10];


/*
 * Return	0 - FAIL
 * 			1 - PASS
 */
int callGNSS(const unsigned char* GNSSin) {
	while(NMEACRCCal(GNSSin) == 1) {


		Delimiter((char *)GNSSin, ',', 2, 80, (unsigned char *)status);
		if(*status == 'A') {

			Delimiter((char *)GNSSin, ',', 3, 80, latTemp); // lat
			Delimiter((char *)GNSSin, ',', 5, 80, lonTemp); // lon

			return 1;
		}else {
			return 0;
		}
	}
	return 0;
}

/*
 * GNSSprocess.h
 *
 *  Created on: Dec 18, 2023
 *      Author: REDWOLF DiGiTAL
 */

#ifndef INC_GNSSPROCESS_H_
#define INC_GNSSPROCESS_H_

extern unsigned char latTemp[16];
extern unsigned char lonTemp[16];

unsigned char callGNSS(void);

void NMEAdecode(const char* nmeaIn, char* out, unsigned char isLat);

#endif /* INC_GNSSPROCESS_H_ */

/*
 * GNSSprocess.h
 *
 *  Created on: Dec 18, 2023
 *      Author: REDWOLF DiGiTAL
 */

#ifndef INC_GNSSPROCESS_H_
#define INC_GNSSPROCESS_H_

extern unsigned char latTemp[10];
extern unsigned char lonTemp[10];

int callGNSS(const unsigned char* GNSSin);

#endif /* INC_GNSSPROCESS_H_ */

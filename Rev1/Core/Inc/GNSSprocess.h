/*
 * GNSSprocess.h
 *
 *  Created on: Dec 18, 2023
 *      Author: REDWOLF DiGiTAL
 */

#ifndef INC_GNSSPROCESS_H_
#define INC_GNSSPROCESS_H_

//extern unsigned char latTemp[16];
//extern unsigned char lonTemp[16];

unsigned char callGNSS(char* lat_out, char* lon_out, char* timeS_out, char* dateS_out);
void NMEAdecoder(char* NMEAin_C, char* dir, char* out);
void NMEAgetTime(char* time_in, char* out);
void NMEAgetDate(char* date_in, char* out);

#endif /* INC_GNSSPROCESS_H_ */

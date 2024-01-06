/*
 * LTEinit.h
 *
 *  Created on: Dec 19, 2023
 *      Author: REDWOLF DiGiTAL
 */

#ifndef INC_LTEDRIVER_H_
#define INC_LTEDRIVER_H_

void initLTE(void);
unsigned char SHUTDOWN_LTE(void);
unsigned char networkRegStatus(void);
void initPDP(uint8_t contextID);
void deactivatePDP(uint8_t contextID);

#endif /* INC_LTEDRIVER_H_ */

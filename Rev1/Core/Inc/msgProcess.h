/*
 * msgProcess.h
 *
 *  Created on: Nov 24, 2023
 *      Author: REDWOLF DiGiTAL (C. Worakan)
 */

#ifndef INC_MSGPROCESS_H_
#define INC_MSGPROCESS_H_

void Delimiter(char *inputData, const char delims, int index, unsigned int maxIndexSize, unsigned char *dataOutput);
unsigned char NMEACRCCal(const unsigned char *sentenceIn);


#endif /* INC_MSGPROCESS_H_ */

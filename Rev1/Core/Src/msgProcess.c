/*
 * msgProcess.c
 *
 * original source code by MangMuang's Elektronik modify and port for STM32 system
 * by REDWOLF DiGiTAL
 *
 *  Created on: Nov 24, 2023
 *  Author: REDWOLF DiGiTAL (C. Worakan)
 */

#include "stm32f4xx_hal.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>


/* =========================================================
 *  MPARSEDELIM by MangMuang's Elektronik
 *  Copyright 2022 (C) MangMuang's Elektronik
 *
 *  This program is sole property of MangMuang's Elektronik
 *  Copy-Distribute-Modify is prohibited
 *
 *  Code created by MangMuang's Elektronik, 10 JULY 2022
 *
 */ // =====================================================
//void Delimiter(char *inputData, const char delims, int index, unsigned int maxIndexSize, unsigned char *dataOutput){
//    // Parsed string(char array) to split value by demiliter and output value
//    // Argument: ( input data , delimiter/splitter letter , no of extract data , max array size, array to store result)
//    unsigned int found = 0;
//    unsigned int strIndex[2] = {0};
//    unsigned int readerPos = 0;
//    unsigned int loaderPos = 0;
//    unsigned int writerPos = 0;
//    for(readerPos = 0; readerPos < maxIndexSize && found <= index; readerPos++){
//        if(*(inputData+readerPos) == delims || readerPos == maxIndexSize){
//            found++;
//            strIndex[0] = strIndex[1]+1;
//            strIndex[1] = (readerPos == maxIndexSize) ? readerPos + 1 : readerPos;
//        }
//    }
//    if(found > index){
//        for(loaderPos = strIndex[0]; loaderPos <strIndex[1]; loaderPos++){
//           *(dataOutput+writerPos) = *(inputData+loaderPos);
//           writerPos++;
//           if((loaderPos > maxIndexSize)||(writerPos > maxIndexSize)){
//               break;
//           }
//        }
//    }else{
////        for(writerPos = 0; writerPos < maxDataOut; writerPos++){
////            *(dataOutput+writerPos) = '\0';
////        }
//    }
//}

void Delimiter(char *inputData, const char delims, int index, unsigned int maxIndexSize, unsigned char *dataOutput){
    // Parsed string(char array) to split value by demiliter and output value
    // Argument: ( input data , delimiter/splitter letter , no of extract data , max array size, array to store result)
    unsigned int found = 0;
    unsigned int strIndex[2] = {0};
    unsigned int readerPos = 0;
    unsigned int loaderPos = 0;
    unsigned int writerPos = 0;

    for(readerPos = 0; readerPos < maxIndexSize && found <= index; readerPos++){

        if(*(inputData+readerPos) == delims || readerPos == maxIndexSize){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (readerPos == maxIndexSize) ? readerPos + 1 : readerPos;
        }
    }

    if(found > index){
        for(loaderPos = strIndex[0]; loaderPos <strIndex[1]; loaderPos++){
           *(dataOutput+writerPos) = *(inputData+loaderPos);
           writerPos++;
           if((loaderPos > maxIndexSize)||(writerPos > maxIndexSize)){
               break;
           }
        }
    }else{
//        for(writerPos = 0; writerPos < sizeof(dataOutput); writerPos++){
//            *(dataOutput+writerPos) = '\0';
//        }
    }


// for fix problem not read first and last positions of string.
// Original source code not considering because it not found delimiter
//
// it have a little bug in case index == 0 data it be not arranged correctly
// but if delimit only one character it work perfectly
// Ex : ABCD123456 -> BCD123456A
//
// to next people if you need to fix it let's do it and thank you (>w<)
//
// Modify by REDWOLF DiGiTAL
// (yep... I'm ignore code performance because I'm not time ;w;)

    if(index == 0) {
        for(loaderPos = 0; loaderPos < strIndex[0]; loaderPos++){
           *(dataOutput+writerPos) = *(inputData+loaderPos);
           writerPos++;

           if((loaderPos > strIndex[0])||(writerPos > strIndex[0])){
               break;
           }
        }
    }

    if(index >= found) {
        for(loaderPos = strIndex[1]+1; *(inputData+loaderPos) != '\0'; loaderPos++){
           *(dataOutput+writerPos) = *(inputData+loaderPos);

           if(*(inputData+loaderPos) == '\0' || (loaderPos >= maxIndexSize) || writerPos >= maxIndexSize) {
               break;
           }

           writerPos++;
        }
    }
}



/* =========================================================
 *  MPARSEDELIM by MangMuang's Elektronik
 *  Copyright 2022 (C) MangMuang's Elektronik
 *
 *  This program is sole property of MangMuang's Elektronik
 *  Copy-Distribute-Modify is prohibited
 *
 *  Code created by MangMuang's Elektronik, 15 JULY 2022
 *
 */ // =====================================================
unsigned char NMEACRCCal(const unsigned char *sentenceIn){
    unsigned char CRCinStr[3] = {0};
    unsigned char CRCin = 0;
    unsigned char CRCResult = 0;
    char* dollarSign;
    char* starSign;
    unsigned int totalSize = 0;
    dollarSign = strchr((const char *)sentenceIn, '$');
    starSign = strchr((const char *)sentenceIn, '*');
    totalSize = starSign-dollarSign;
    strncpy((char *)CRCinStr, starSign + 1, 2);
    CRCin = (unsigned char)strtol((char *)CRCinStr, NULL, 16);
    for(unsigned int CalCnt = 1; CalCnt < totalSize; CalCnt++){
        CRCResult ^= sentenceIn[CalCnt];
    }
    unsigned char CRCCMPResult = 0;
    if(CRCin == CRCResult){
        CRCCMPResult = 1;
    }
    return CRCCMPResult;

}

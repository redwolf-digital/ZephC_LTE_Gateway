/*
 * initernet.h
 *
 *  Created on: Jan 8, 2024
 *      Author: MOMIJI
 */

#ifndef INC_INITERNET_H_
#define INC_INITERNET_H_

void clearText_net_Temp(void);

unsigned char AckInternet(void);
unsigned int gpsSend(char* lat, char* lon, char* Time_S, char* Date_S, char* out);
unsigned int httpSend(char* lat, char* lon, char* device_ID, char* time_s, char* date_s, char* x, char* y, char* z, char* humi, char* temp, char* eth, char* carbon, char* airflow, char* out);
unsigned char ping(void);
unsigned char Activate(void);
unsigned char Deactivate(void);

void addZero(char* in, char* out);

#endif /* INC_INITERNET_H_ */

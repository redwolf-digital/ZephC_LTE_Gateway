/*
Firmware for make boot sequnce and Ext. WDT for ZephC

PROCESSOR..............................Atmel AVR attiny13/13A
FIRMWARE VERSION.......................1.0
AUTHOR.................................REDWOLF DiGiTAL [C.Worakan]
*/

#include <AVR/io.h>
#include <AVR/interrupt.h>
#include <util/delay.h>


void LTE_Pulse(void);
void ESP_Pulse(void);


#define LTE_RST       PB4
#define ESP_RST       PB3
#define HB            PB1

#define WD_TIMEOUT_S  35

volatile unsigned int TIM_OVF_COUNTER = 0;
volatile unsigned char WD_TIME_COUNT = 0;

ISR(TIM0_OVF_vect) {
  if(++TIM_OVF_COUNTER > 37) {
    WD_TIME_COUNT++;
    TIM_OVF_COUNTER = 0;
  }
}

ISR(INT0_vect) {
  WD_TIME_COUNT = 0;
}


int main(void) {
  // Clock config
  CLKPR |= (1 << CLKPS1) | (1 << CLKPS0);

  // Init pin INPUT
  DDRB &= ~(1 << HB);

  // Init pin OUTPUT
  DDRB |= (1 << LTE_RST);
  DDRB |= (1 << ESP_RST);

  // Set boot output state
  PORTB &= ~(1 << LTE_RST);
  PORTB &= ~(1 << ESP_RST);

  // Interrupt service
  cli();

  // Start boot seq.
  _delay_ms(1000);
  ESP_Pulse();
  _delay_ms(150);
  LTE_Pulse();

  
  // Interrupt service
  // Timer 0
  TCCR0B |= (1 << CS02) | (1 << CS00);
  TIMSK0 |= (1 << TOIE0);

  MCUCR |= (1 << ISC01);
  GIMSK |= (1 << INT0);

  sei();



  while(1) {

    while(WD_TIME_COUNT == WD_TIMEOUT_S) {
      _delay_ms(1000);
      ESP_Pulse();
      _delay_ms(150);
      LTE_Pulse();
      WD_TIME_COUNT = 0;
    }

  }
}


void LTE_Pulse(void) {
  PORTB |= (1 << LTE_RST);
  _delay_ms(1);
  PORTB &= ~(1 << LTE_RST);
}

void ESP_Pulse(void) {
  PORTB |= (1 << ESP_RST);
  _delay_ms(1);
  PORTB &= ~(1 << ESP_RST);
}

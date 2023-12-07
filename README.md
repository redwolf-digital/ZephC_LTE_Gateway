# ZephC LTE Gateway

### Processor
- STM32F411EC (ARM Cortex M4)
- Freq. : 72 MHz
- RAM : 128 KB
- FLASH : 512 KB

### Software
- STM32CubeUDE

### Status port
| Pin name | Pin descriptins | Port | Pin | Mode |
| :-- | :-- | :-: | :-- | :--
Debug | general purpose output port for test <br> *normally not used* | B | 1 | OUTPUT |
BUSY | for tell other device now gateway is busy | B | 5 | OUTPUT |
ONLINE | for tell other device now gateway is online or offline | B | 6 | OUTPUT |
RDY | for tell other device now gateway is ready to receive data | B | 7 | OUTPUT |
RTS | for tell gateway wait to receive data | B | 8 | INPUT *Interrupt* |
ERROR | if system have error | B | 9 | OUTPUT |


### Communications port
***UART1*** - used for communicate between **gateway and master** via UART to ***RS485***
| Descriptions | setting |
| :-- | :-- |
Baud rate | 9600 |
Word length | 8 bit |
Parity | None |
Stop bit | 1 |

| Pin descriptins | Port | Pin |
| :-- | :-: | :-- |
Tx | A | 9 |
Rx | A | 10 |


***UART2*** - used for communicate between **gateway and LTE module** via UART
| Descriptions | setting |
| :-- | :-- |
Baud rate | 115200 |
Word length | 8 bit |
Parity | None |
Stop bit | 1 |

| Pin descriptins | Port | Pin |
| :-- | :-: | :-- |
Tx | A | 2 |
Rx | A | 3 |

***USB (VCOM)*** - used for debug only  
*you can used any serial terminal software and baud rate*
| Pin descriptins | Port | Pin |
| :-- | :-: | :-- |
USB DP | A | 12 |
USB DM | A | 11 |


### Data frame

| **Name** | BOX ID | HH/MM/SS | DD/MM/YY | X | Y | Z | HUMI | TEMP | MQ3 |CO2 | AIRFLOW | Q
| --- | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| **Data type** | Str | Str | Str | Float | Float | Float | Float | Float | Float | Float | Float | Str |

***ex :***   
A,17/39/00,7/12/23,123.45,123.45,123.45,99.00,3.55,0.0,0.32,0.45,Q  
B,,,,,,,,,,,  

/*
 * uart.h
 *
 *  Created on: 2021Äê6ÔÂ24ÈÕ
 *      Author: 14472
 */

#ifndef UART_H_
#define UART_H_

#include "DSP2833x_Device.h"    
#include "DSP2833x_Examples.h" 


#define UART_AUTOBAUN_TEST

void UARTa_Init(Uint32 baud);
void UARTa_SendByte(int a);
void UARTa_SendString(char * msg);
void UARTa_SendUint16(Uint16 data);
void UARTa_SendNumber(int32 number);
void UARTa_SendHex(Uint16 number);
void UARTa_SendStringAndNumber(char * msg1, int32 number, char * msg2);
void UARTa_SendStringAndHex(char * msg1, Uint16 number, char * msg2);

void UART_AutoBaud_Test(void);

#endif /* UART_H_ */

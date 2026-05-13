/*
 * timer.h
 *
 *  Created on: 2021年6月4日
 *      Author: 14472
 */

#ifndef TIMER_H_
#define TIMER_H_


#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件


// Timer 0 initialization function
// Freq: CPU clock frequency (150MHz)
// Period: Timer period value, unit: us
// isr: the interrupt service routine
void TIM0_Init(float Freq, float Period, PINT isr);

#endif /* TIMER_H_ */







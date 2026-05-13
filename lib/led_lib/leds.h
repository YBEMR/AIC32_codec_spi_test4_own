/*
 * leds.h
 *
 *  Created on: 2021年5月19日
 *      Author: 14472
 */

#ifndef _LEDS_H_
#define _LEDS_H_



#include  "DSP2833x_Device.h"   //  DSP2833x  头文件
#include  "DSP2833x_Examples.h" //  DSP2833x  例子相关头文件


#define  LED4_OFF   (GpioDataRegs.GPASET.bit.GPIO16=1)
#define  LED4_ON    (GpioDataRegs.GPACLEAR.bit.GPIO16=1)
#define  LED4_TOGGLE    (GpioDataRegs.GPATOGGLE.bit.GPIO16=1)



#define  LED3_OFF   (GpioDataRegs.GPASET.bit.GPIO15=1)
#define  LED3_ON    (GpioDataRegs.GPACLEAR.bit.GPIO15=1)
#define  LED3_TOGGLE    (GpioDataRegs.GPATOGGLE.bit.GPIO15=1)



#define  LED2_OFF   (GpioDataRegs.GPBSET.bit.GPIO61=1)
#define  LED2_ON    (GpioDataRegs.GPBCLEAR.bit.GPIO61=1)
#define  LED2_TOGGLE    (GpioDataRegs.GPBTOGGLE.bit.GPIO61=1)


#define  LED1_OFF   (GpioDataRegs.GPBSET.bit.GPIO60=1)
#define  LED1_ON    (GpioDataRegs.GPBCLEAR.bit.GPIO60=1)
#define  LED1_TOGGLE    (GpioDataRegs.GPBTOGGLE.bit.GPIO60=1)



void  LED_Init(void);




#endif /* _LEDS_H_ */

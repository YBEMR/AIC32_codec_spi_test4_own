/*
 * leds.c
 *
 *  Created on: 2021年5月19日
 *      Author: 14472
 */

#include <leds.h>


void  LED_Init(void)
{

EALLOW;


//LED11 端口配置
GpioCtrlRegs.GPAMUX2.bit.GPIO16=0;
GpioCtrlRegs.GPADIR.bit.GPIO16=1;
GpioCtrlRegs.GPAPUD.bit.GPIO16=0;

//LED10 端口配置
GpioCtrlRegs.GPAMUX1.bit.GPIO15=0;
GpioCtrlRegs.GPADIR.bit.GPIO15=1;
GpioCtrlRegs.GPAPUD.bit.GPIO15=0;

//LED9端口配置
GpioCtrlRegs.GPBMUX2.bit.GPIO61=0;
GpioCtrlRegs.GPBDIR.bit.GPIO61=1;
GpioCtrlRegs.GPBPUD.bit.GPIO61=0;

//LED8 端口配置
GpioCtrlRegs.GPBMUX2.bit.GPIO60=0;
GpioCtrlRegs.GPBDIR.bit.GPIO60=1;
GpioCtrlRegs.GPBPUD.bit.GPIO60=0;

GpioDataRegs.GPASET.bit.GPIO16=1;
GpioDataRegs.GPASET.bit.GPIO15=1;
GpioDataRegs.GPBSET.bit.GPIO61=1;
GpioDataRegs.GPBSET.bit.GPIO60=1;


EDIS;
}

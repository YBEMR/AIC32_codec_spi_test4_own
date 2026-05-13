/*
 * exint.c
 *
 *  Created on: June 3, 2021
 *      Author: 14472
 */

#include "exint.h"
#include <stdint.h>


void EXINT1_Init(PINT ISR_FUNC)
{
	EALLOW;
	SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;    // GPIO input clock
	EDIS;

	EALLOW;
	// KEY port configuration
	GpioCtrlRegs.GPAMUX1.bit.GPIO12=0;
	GpioCtrlRegs.GPADIR.bit.GPIO12=0;
	GpioCtrlRegs.GPAPUD.bit.GPIO12=0;
	GpioCtrlRegs.GPAQSEL1.bit.GPIO12 = 0;        // External interrupt 1 (XINT1) synchronized with system clock SYSCLKOUT

#if TEST_MODE == 1
	GpioCtrlRegs.GPBMUX2.bit.GPIO49=0;
	GpioCtrlRegs.GPBDIR.bit.GPIO49=1;
	GpioCtrlRegs.GPBPUD.bit.GPIO49=0;
	GpioDataRegs.GPBCLEAR.bit.GPIO49=1;
#else 
	GpioCtrlRegs.GPBMUX2.bit.GPIO48=0;
	GpioCtrlRegs.GPBDIR.bit.GPIO48=1;
	GpioCtrlRegs.GPBPUD.bit.GPIO48=0;
	GpioDataRegs.GPBCLEAR.bit.GPIO48=1;
#endif
	EDIS;

	EALLOW;
	GpioIntRegs.GPIOXINT1SEL.bit.GPIOSEL = 12;   // XINT1 is GPIO12
	EDIS;

	EALLOW;	// Modify protected registers, EALLOW statement should be added before modification
	PieVectTable.XINT1 = ISR_FUNC;
	EDIS;   // EDIS means modification of protected registers is not allowed

	PieCtrlRegs.PIEIER1.bit.INTx4 = 1;          // Enable INT4 of PIE Group 1

	XIntruptRegs.XINT1CR.bit.POLARITY = 3;      // Interrupt triggered on both edges
	XIntruptRegs.XINT1CR.bit.ENABLE= 1;         // Enable XINT1

	IER |= M_INT1;                              // Enable CPU interrupt 1 (INT1)
//	EINT;                                       // Enable global interrupts
//	ERTM;
}

void EXINT2_Init(PINT ISR_FUNC)
{
	EALLOW;
	SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;    // GPIO input clock
	EDIS;

	EALLOW;
	// KEY port configuration
	GpioCtrlRegs.GPAMUX1.bit.GPIO13=0;
	GpioCtrlRegs.GPADIR.bit.GPIO13=0;
	GpioCtrlRegs.GPAPUD.bit.GPIO13=0;
	GpioCtrlRegs.GPAQSEL1.bit.GPIO13 = 2;       // External interrupt 2 (XINT2) input qualification 6 sampling windows
	GpioCtrlRegs.GPACTRL.bit.QUALPRD1 = 0xFF;   // Period of each sampling window is 510*SYSCLKOUT

	GpioCtrlRegs.GPBMUX2.bit.GPIO48=0;
	GpioCtrlRegs.GPBDIR.bit.GPIO48=1;
	GpioCtrlRegs.GPBPUD.bit.GPIO48=0;
	GpioDataRegs.GPBCLEAR.bit.GPIO48=1;
	EDIS;

	EALLOW;
	GpioIntRegs.GPIOXINT2SEL.bit.GPIOSEL = 13;   // XINT2 is GPIO13
	EDIS;

	EALLOW;	// Modify protected registers, EALLOW statement should be added before modification
	PieVectTable.XINT2 = ISR_FUNC;
	EDIS;   // EDIS means modification of protected registers is not allowed

	PieCtrlRegs.PIEIER1.bit.INTx5 = 1;          // Enable INT5 of PIE Group 1

	XIntruptRegs.XINT2CR.bit.POLARITY = 0;      // Interrupt triggered on falling edge
	XIntruptRegs.XINT2CR.bit.ENABLE = 1;        // Enable XINT2

	IER |= M_INT1;                              // Enable CPU interrupt 1 (INT1)
//	EINT;                                       // Enable global interrupts
//	ERTM;
}


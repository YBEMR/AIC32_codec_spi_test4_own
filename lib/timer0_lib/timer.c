/*
 * timer.c
 *
 *  Created on: 2021Äê6ÔÂ4ÈÕ
 *      Author: 14472
 */

#include "timer.h"
#include <stdint.h>

// Timer 0 initialization function
// Freq: CPU clock frequency (150MHz)
// Period: Timer period value, unit: us
void TIM0_Init(float Freq, float Period, PINT isr)
{
	EALLOW;
	SysCtrlRegs.PCLKCR3.bit.CPUTIMER0ENCLK = 1; // CPU Timer 0
	EDIS;

	// Set the interrupt entry address of Timer 0 to INT0 in the interrupt vector table
	EALLOW;
	PieVectTable.TINT0 = isr;  // Use the parameter "isr" (either as a function name or "&" is acceptable)
	EDIS;

	// Point to the register address of Timer 0
	CpuTimer0.RegsAddr = &CpuTimer0Regs;
	// Set the period register value of Timer 0
	CpuTimer0Regs.PRD.all  = 0xFFFFFFFF;
	// Set the timer prescaler counter value to 0
	CpuTimer0Regs.TPR.all  = 0;
	CpuTimer0Regs.TPRH.all = 0;
	// Ensure Timer 0 is in stop state
	CpuTimer0Regs.TCR.bit.TSS = 1;
	// Reload enable
	CpuTimer0Regs.TCR.bit.TRB = 1;
	// Reset interrupt counters:
	CpuTimer0.InterruptCount = 0;

	ConfigCpuTimer(&CpuTimer0, Freq, Period);

	// Start timer function
	CpuTimer0Regs.TCR.bit.TSS=0;
	// Enable CPU interrupt group 1 and enable the 7th sub-interrupt of group 1, which is Timer 0
	IER |= M_INT1;
	PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
	// Enable global interrupts
//	EINT;
//	ERTM;

}

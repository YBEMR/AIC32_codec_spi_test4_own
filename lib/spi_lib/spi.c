#include "spi.h"

void spi_init()
{
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1; // Enable SPIA clock
	EDIS;

	InitSpiaGpio();
	SpiaRegs.SPICCR.all =0x000F;	             			// Reset on, rising edge, 16-bit char bits
	                                             			// 0x000F corresponds to Rising Edge, 0x004F corresponds to Falling Edge
	SpiaRegs.SPICTL.all =0x0006;    		     			// Enable master mode, normal phase,

	// enable talk, and SPI int disabled.
//	SpiaRegs.SPIBRR =0x007F;
	SpiaRegs.SPIBRR =0x0025;
    SpiaRegs.SPICCR.all =0x008F;		         			// Relinquish SPI from Reset
    SpiaRegs.SPIPRI.bit.FREE = 1;                			// Set so breakpoints don't disturb xmission
}

void spi_ready_init(PINT isr)
{
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;    // Enable GPIO input clock
    EDIS;

    EALLOW;
    // GPIO50 Configuration
    GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO50 = 0;         // Input
    GpioCtrlRegs.GPBPUD.bit.GPIO50 = 0;         // Enable pull-up

    // Add filtering to avoid false triggering
    GpioCtrlRegs.GPBQSEL2.bit.GPIO50 = 2;       // 6-cycle filter
    GpioCtrlRegs.GPBCTRL.bit.QUALPRD2 = 0xFF;   // Increase sampling interval
    EDIS;

    EALLOW;
    // --- XINT3 Mapping ---
    // XINT3 for F28335 corresponds to PortB, need to subtract 32 from the value
    GpioIntRegs.GPIOXINT3SEL.bit.GPIOSEL = 50 - 32;
    EDIS;

    EALLOW;
    PieVectTable.XINT3 = isr;
    EDIS;

    // --- Enable PIE Group 12 ---
    // Clear flag first to prevent entering interrupt immediately after enabling
    PieCtrlRegs.PIEIFR12.bit.INTx1 = 0;
    PieCtrlRegs.PIEIER12.bit.INTx1 = 1;

    // Trigger mode
    XIntruptRegs.XINT3CR.bit.POLARITY = 1;      // 1：rising edge, 0：falling edge，3：both edges
    XIntruptRegs.XINT3CR.bit.ENABLE = 1;

    // Enable CPU INT12
    IER |= M_INT12;

    // GPIO10输入，GPIO11输出
    // GPIO50 SLAVE_DATA_READY
    // GPIO10 SLAVE_SPI_READY
    // GPIO11 MASTER_DATA_REQ
    // 空闲时均为高电平

    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO10=0;
    GpioCtrlRegs.GPADIR.bit.GPIO10=0;
    GpioCtrlRegs.GPAPUD.bit.GPIO10=0;

    GpioCtrlRegs.GPAMUX1.bit.GPIO11=0;
    GpioCtrlRegs.GPADIR.bit.GPIO11=1;
    GpioCtrlRegs.GPAPUD.bit.GPIO11=0;

    GpioDataRegs.GPASET.bit.GPIO11=1;
    EDIS;
}

void spi_xon_init(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;    // Enable GPIO input clock
    EDIS;

    // GPIO50 输入，GPIO10输入，GPIO11输出
    // GPIO50 SLAVE_DATA_READY
    // GPIO10 SLAVE_SPI_READY
    // GPIO11 MASTER_DATA_REQ
    // 空闲时均为高电平
    EALLOW;
    // GPIO50 Configuration
    GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO50 = 0;         // Input
    GpioCtrlRegs.GPBPUD.bit.GPIO50 = 0;         // Enable pull-up

    GpioCtrlRegs.GPAMUX1.bit.GPIO10=0;
    GpioCtrlRegs.GPADIR.bit.GPIO10=0;
    GpioCtrlRegs.GPAPUD.bit.GPIO10=0;

    GpioCtrlRegs.GPAMUX1.bit.GPIO11=0;
    GpioCtrlRegs.GPADIR.bit.GPIO11=1;
    GpioCtrlRegs.GPAPUD.bit.GPIO11=0;

    GpioDataRegs.GPASET.bit.GPIO11=1;
    EDIS;
}

void spi_send_and_receive(const Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length)
{
    Uint16 sent_count = 0;
    Uint16 recv_count = 0;

    // Enable TX FIFO
    SpiaRegs.SPIFFTX.bit.SPIFFENA = 1;

    // Release RX FIFO reset
    SpiaRegs.SPIFFRX.bit.RXFIFORESET = 1;

    while ((sent_count < length) || (recv_count < length))
    {
        // Fill TX FIFO, max 16 data words
        while ((SpiaRegs.SPIFFTX.bit.TXFFST < 16) && (sent_count < length))
        {
            SpiaRegs.SPITXBUF = send_buffer[sent_count];
            sent_count++;
        }

        while (SpiaRegs.SPIFFTX.bit.TXFFST > 0) {}  // Block waiting for TX completion


        // Read RX FIFO
        while ((SpiaRegs.SPIFFRX.bit.RXFFST > 0) && (recv_count < length))
        {
            receive_buffer[recv_count] = SpiaRegs.SPIRXBUF;
            recv_count++;
        }
    }

    // Disable TX/RX FIFO
    SpiaRegs.SPIFFTX.bit.SPIFFENA = 0;

    // Reset TX FIFO
    // SpiaRegs.SPIFFTX.bit.TXFIFO = 1;

    // Reset RX FIFO to clear it
    SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;
    SpiaRegs.SPIFFRX.bit.RXFFOVFCLR = 1;
}

void spi_send_bulk(Uint16* buffer, Uint16 *receive_buffer, Uint16 length)
{
    Uint16 sent_count = 0;
    Uint16 recv_count = 0;
    SpiaRegs.SPIFFTX.bit.SPIFFENA = 1;
    while (sent_count < length)
    {
        // Fill FIFO, max 16 data words
        while (SpiaRegs.SPIFFTX.bit.TXFFST < 16 && sent_count < length)
        {
            SpiaRegs.SPITXBUF = buffer[sent_count];
            sent_count++;
        }
        while (SpiaRegs.SPIFFTX.bit.TXFFST > 0) {}
    }

    // Read RX FIFO (as long as there is data and not fully received)
    while ((SpiaRegs.SPIFFRX.bit.RXFFST > 0))
    {
        receive_buffer[recv_count] = SpiaRegs.SPIRXBUF;
        recv_count++;
        if(recv_count >= length){
            recv_count = 0;
        }
    }

    SpiaRegs.SPIFFTX.bit.SPIFFENA = 0;
}

/**
 * @brief SPI Loopback Test Function
 * This function sends data through SPI and receives it back to verify the integrity of the transmission.
 * @param send_buffer Pointer to the buffer containing data to be sent.
 * @param receive_buffer Pointer to the buffer where received data will be stored.
 * @param length Number of data words to send and receive.
 * @return int16_t Returns 0 if the test passes (data matches), -1 if there is a mismatch.
 */
int16 spi_lookback_test(Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length)
{
    spi_send_and_receive((Uint16*)send_buffer, (Uint16*)receive_buffer, length);

    // Verify data
    for(Uint16 i = 0; i < length; i++)
    {
        if(send_buffer[i] != receive_buffer[i])
        {
            return -1; // Data mismatch
        }
    }
    return 0; // Success

}

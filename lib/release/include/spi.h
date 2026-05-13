#ifndef _SPI_H_
#define _SPI_H_

#include "DSP2833x_Device.h"    
#include "DSP2833x_Examples.h"

#define SPI_DATASIZE_8BIT  0
#define SPI_DATASIZE_16BIT 1
#define SPI_SLAVE_DATASIZE SPI_DATASIZE_16BIT

void spi_init(void);
void spi_ready_init(PINT isr);
void spi_send_and_receive(const Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length);
void spi_send_bulk(Uint16* buffer, Uint16 *receive_buffer, Uint16 length);
int16 spi_lookback_test(Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length);
#endif  // _SPI_H_









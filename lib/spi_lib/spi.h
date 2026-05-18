#ifndef _SPI_H_
#define _SPI_H_

#include "DSP2833x_Device.h"    
#include "DSP2833x_Examples.h"

#define SPI_DATASIZE_8BIT  0
#define SPI_DATASIZE_16BIT 1
#define SPI_SLAVE_DATASIZE SPI_DATASIZE_16BIT

#define SPI_PTT_OK              0
#define SPI_PTT_ERR_TIMEOUT    -1

#define SPI_PTT_DSP_REQ_GPIO      51U
#define SPI_PTT_SPI_READY_GPIO    50U
#define SPI_PTT_DATA_READY_GPIO   52U

void spi_init(void);
void spi_ready_init(PINT isr);
void spi_ptt_gpio_init(void);
void spi_ptt_set_dsp_req(Uint16 level);
Uint16 spi_ptt_is_spi_ready(void);
Uint16 spi_ptt_is_data_ready(void);
int16 spi_ptt_wait_spi_ready(Uint32 timeout_loop);
int16 spi_ptt_wait_data_ready(Uint32 timeout_loop);
void spi_send_and_receive(const Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length);
void spi_send_bulk(Uint16* buffer, Uint16 *receive_buffer, Uint16 length);
int16 spi_lookback_test(Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length);
#endif  // _SPI_H_









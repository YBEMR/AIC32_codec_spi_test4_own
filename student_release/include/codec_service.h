#ifndef CODEC_SERVICE_H_
#define CODEC_SERVICE_H_

#include <stdint.h>
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

#define CODEC_SERVICE_MAX_RECORD_CNT   40000UL
#define CODEC_SERVICE_AMR_BUF_SIZE     (CODEC_SERVICE_MAX_RECORD_CNT + 1UL)
#define CODEC_SERVICE_SPI_PACKET_SIZE  CODEC_SERVICE_AMR_BUF_SIZE

#define CODEC_SERVICE_OK                 0
#define CODEC_SERVICE_ERR_NO_RECORD     -1
#define CODEC_SERVICE_ERR_ENCODE        -2
#define CODEC_SERVICE_ERR_DECODE        -3
#define CODEC_SERVICE_ERR_LENGTH        -4
#define CODEC_SERVICE_PLAY_DONE          1

#define SPI_DATASIZE_8BIT  0
#define SPI_DATASIZE_16BIT 1
#define SPI_SLAVE_DATASIZE SPI_DATASIZE_16BIT

void codec_service_reset(void);
void codec_service_start_record(void);
int16_t codec_service_record_sample(int16_t sample);
Uint32 codec_service_get_record_count(void);

int16_t codec_service_encode_recorded(void);
int16_t codec_service_spi_exchange_first(void);
int16_t codec_service_spi_exchange_second(void);
int16_t codec_service_decode_received(void);
int16_t codec_service_get_play_sample(Uint16 *sample);

Uint16 codec_service_get_amr_len(void);
Uint16 codec_service_get_received_amr_len(void);
Uint32 codec_service_get_pcm_sample_count(void);
Uint32 codec_service_get_play_sample_index(void);

const uint8_t *codec_service_get_amr_buffer(void);
const uint8_t *codec_service_get_spi_rx_buffer(void);
const int16_t *codec_service_get_pcm_buffer(void);

#endif /* CODEC_SERVICE_H_ */

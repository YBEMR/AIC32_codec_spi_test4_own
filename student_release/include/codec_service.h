#ifndef CODEC_SERVICE_H_
#define CODEC_SERVICE_H_

#include <stdint.h>
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

// #define CODEC_SERVICE_MAX_RECORD_CNT   40000U
// #define CODEC_SERVICE_AMR_BUF_SIZE     1500U
// #define CODEC_SERVICE_SPI_PACKET_SIZE  1500U

#define CODEC_SERVICE_MAX_RECORD_CNT   140000U
#define CODEC_SERVICE_AMR_BUF_SIZE     8192U
#define CODEC_SERVICE_SPI_PACKET_SIZE  8192U

#define CODEC_SERVICE_OK                 0
#define CODEC_SERVICE_ERR_NO_RECORD     -1
#define CODEC_SERVICE_ERR_ENCODE        -2
#define CODEC_SERVICE_ERR_DECODE        -3
#define CODEC_SERVICE_ERR_LENGTH        -4
#define CODEC_SERVICE_ERR_TIMEOUT       -5
#define CODEC_SERVICE_PLAY_DONE          1
#define CODEC_SERVICE_PTT_DONE           2

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
int16_t codec_service_ptt_spi_status_probe(void);
int16_t codec_service_decode_received(void);
int16_t codec_service_get_play_sample(Uint16 *sample);

int16_t codec_service_ptt_encode_begin(void);
int16_t codec_service_ptt_encode_next(uint8_t *amr_frame,
                                      Uint16 amr_frame_buf_size,
                                      Uint16 *amr_frame_len,
                                      Uint16 *frame_id,
                                      Uint16 *frame_count,
                                      Uint16 *is_last);

int16_t codec_service_ptt_decode_begin(void);
int16_t codec_service_ptt_decode_frame(const uint8_t *amr_frame,
                                       Uint16 amr_frame_len);
int16_t codec_service_ptt_decode_finish(void);

Uint16 codec_service_get_amr_len(void);
Uint16 codec_service_get_received_amr_len(void);
Uint32 codec_service_get_play_sample_count(void);

const uint8_t *codec_service_get_amr_buffer(void);
const uint8_t *codec_service_get_spi_rx_buffer(void);
const Uint16 *codec_service_get_ptt_spi_rx_words(void);

#endif /* CODEC_SERVICE_H_ */

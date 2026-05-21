#ifndef CODEC_SERVICE_H_
#define CODEC_SERVICE_H_

#include <stdint.h>
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include "g711_codec.h"

#define CODEC_SERVICE_MAX_RECORD_CNT   40000UL
#define CODEC_SERVICE_G711_BUF_SIZE    (CODEC_SERVICE_MAX_RECORD_CNT + 1UL)
#define CODEC_SERVICE_SPI_PACKET_SIZE  CODEC_SERVICE_G711_BUF_SIZE

#define CODEC_SERVICE_STREAM_FRAME_SAMPLES            G711_FRAME_SAMPLES
#define CODEC_SERVICE_STREAM_FRAME_OCTETS             G711_FRAME_OCTETS
#define CODEC_SERVICE_STREAM_ENCODED_FRAME_CAPACITY   4U
#define CODEC_SERVICE_STREAM_PLAY_FRAME_CAPACITY      4U

#define CODEC_SERVICE_OK                 0
#define CODEC_SERVICE_ERR_NO_RECORD     -1
#define CODEC_SERVICE_ERR_ENCODE        -2
#define CODEC_SERVICE_ERR_DECODE        -3
#define CODEC_SERVICE_ERR_LENGTH        -4
#define CODEC_SERVICE_ERR_OVERFLOW      -5
#define CODEC_SERVICE_ERR_UNDERFLOW     -6
#define CODEC_SERVICE_ERR_PARAM         -7
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

Uint16 codec_service_get_g711_len(void);
Uint16 codec_service_get_received_g711_len(void);
Uint32 codec_service_get_pcm_sample_count(void);
Uint32 codec_service_get_play_sample_index(void);

const uint8_t *codec_service_get_g711_buffer(void);
const uint8_t *codec_service_get_spi_rx_buffer(void);
const int16_t *codec_service_get_pcm_buffer(void);

/*
 * 20 ms G.711 streaming helpers.
 *
 * Each encoded frame contains 160 Uint16 words; only the low 8 bits of each
 * word hold the G.711 A-law octet. Full queues drop the new frame and count an
 * overflow. Empty queues return CODEC_SERVICE_ERR_UNDERFLOW or
 * CODEC_SERVICE_PLAY_DONE and count an underflow.
 */
void codec_service_stream_reset(void);
void codec_service_stream_start_capture(void);
void codec_service_stream_stop_capture(void);
int16_t codec_service_stream_record_sample(int16_t sample);
Uint16 codec_service_stream_has_encoded_frame(void);
int16_t codec_service_stream_get_encoded_frame(Uint16 *frame_words,
                                               Uint16 max_words,
                                               Uint16 *out_words);
int16_t codec_service_stream_put_play_frame(const Uint16 *g711_words,
                                            Uint16 octets);
int16_t codec_service_stream_get_play_sample(Uint16 *sample);
Uint32 codec_service_stream_get_encoded_frame_count(void);
Uint32 codec_service_stream_get_play_frame_count(void);
Uint32 codec_service_stream_get_overflow_count(void);
Uint32 codec_service_stream_get_underflow_count(void);

#endif /* CODEC_SERVICE_H_ */

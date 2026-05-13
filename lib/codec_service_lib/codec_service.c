#include "codec_service.h"

#include <string.h>

#include "audio.h"
#include "spi.h"

typedef union {
    int16_t record_buf[CODEC_SERVICE_MAX_RECORD_CNT];
    uint8_t wav_output_buffer[CODEC_SERVICE_WAV_BUF_SIZE];
} codec_service_workbuf_t;

#pragma DATA_SECTION(codec_workbuf, "ZONE7DATA");
static codec_service_workbuf_t codec_workbuf;

#pragma DATA_SECTION(amr_output_buffer, "ZONE7DATA");
static uint8_t amr_output_buffer[CODEC_SERVICE_AMR_BUF_SIZE];

#pragma DATA_SECTION(spi_receive_buffer, "ZONE7DATA");
static uint8_t spi_receive_buffer[CODEC_SERVICE_AMR_BUF_SIZE];
static Uint32 record_count = 0;
static Uint16 amr_len = 0;
static Uint16 received_amr_len = 0;
static Uint32 wav_total_len = 0;
static Uint32 play_offset = 44U;

void codec_service_reset(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    wav_total_len = 0;
    play_offset = 44U;
    memset(&codec_workbuf, 0, sizeof(codec_workbuf));
    memset(amr_output_buffer, 0, sizeof(amr_output_buffer));
    memset(spi_receive_buffer, 0, sizeof(spi_receive_buffer));
}

void codec_service_start_record(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    wav_total_len = 0;
    play_offset = 44U;
    memset(&codec_workbuf, 0, sizeof(codec_workbuf));
    memset(amr_output_buffer, 0, sizeof(amr_output_buffer));
    memset(spi_receive_buffer, 0, sizeof(spi_receive_buffer));
}

int16_t codec_service_record_sample(int16_t sample)
{
    if (record_count >= CODEC_SERVICE_MAX_RECORD_CNT) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    codec_workbuf.record_buf[record_count++] = sample;
    return CODEC_SERVICE_OK;
}

Uint32 codec_service_get_record_count(void)
{
    return record_count;
}

int16_t codec_service_encode_recorded(void)
{
    int16_t result;

    if (record_count == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    memset(amr_output_buffer, 0, sizeof(amr_output_buffer));

    result = amr_encode_pcm16(
            codec_workbuf.record_buf,
            record_count,
            &amr_output_buffer[1],
            CODEC_SERVICE_AMR_BUF_SIZE - 1U,
            &amr_len);

    if (result != 0) {
        amr_len = 0;
        return CODEC_SERVICE_ERR_ENCODE;
    }

    amr_output_buffer[0] = (uint8_t)amr_len;
    return CODEC_SERVICE_OK;
}

int16_t codec_service_spi_exchange_first(void)
{
    memset(spi_receive_buffer, 0, sizeof(spi_receive_buffer));
    spi_send_and_receive((const Uint16 *)amr_output_buffer,
                         (Uint16 *)spi_receive_buffer,
                         CODEC_SERVICE_SPI_PACKET_SIZE);
    memset(spi_receive_buffer, 0, sizeof(spi_receive_buffer));
    return CODEC_SERVICE_OK;
}

int16_t codec_service_spi_exchange_second(void)
{
    memset(spi_receive_buffer, 0, sizeof(spi_receive_buffer));
    spi_send_and_receive((const Uint16 *)amr_output_buffer,
                         (Uint16 *)spi_receive_buffer,
                         CODEC_SERVICE_SPI_PACKET_SIZE);

#if SPI_SLAVE_DATASIZE == SPI_DATASIZE_8BIT
    received_amr_len = (((Uint16)spi_receive_buffer[0] & 0x00ffU) << 8) |
                       (((Uint16)spi_receive_buffer[0] >> 8) & 0x00ffU);
#elif SPI_SLAVE_DATASIZE == SPI_DATASIZE_16BIT
    received_amr_len = (Uint16)spi_receive_buffer[0];
#else
#error "SPI_SLAVE_DATASIZE must be 8BIT or 16BIT"
#endif

    if (received_amr_len != amr_len) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    return CODEC_SERVICE_OK;
}

int16_t codec_service_decode_received(void)
{
    int16_t result;

    if (received_amr_len == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    memset(codec_workbuf.wav_output_buffer, 0, sizeof(codec_workbuf.wav_output_buffer));
    result = amr_decode_wav(&spi_receive_buffer[1],
                            received_amr_len,
                            codec_workbuf.wav_output_buffer,
                            CODEC_SERVICE_WAV_BUF_SIZE,
                            &wav_total_len);
    if (result != 0) {
        wav_total_len = 0;
        return CODEC_SERVICE_ERR_DECODE;
    }

    play_offset = 44U;
    return CODEC_SERVICE_OK;
}

int16_t codec_service_get_play_sample(Uint16 *sample)
{
    if ((sample == 0) || (wav_total_len <= 44U) || (play_offset + 1U >= wav_total_len)) {
        return CODEC_SERVICE_PLAY_DONE;
    }

    *sample = (Uint16)codec_workbuf.wav_output_buffer[play_offset] |
              ((Uint16)codec_workbuf.wav_output_buffer[play_offset + 1U] << 8);
    play_offset += 2U;

    if (play_offset >= wav_total_len) {
        return CODEC_SERVICE_PLAY_DONE;
    }

    return CODEC_SERVICE_OK;
}

Uint16 codec_service_get_amr_len(void)
{
    return amr_len;
}

Uint16 codec_service_get_received_amr_len(void)
{
    return received_amr_len;
}

Uint32 codec_service_get_wav_len(void)
{
    return wav_total_len;
}

Uint32 codec_service_get_play_offset(void)
{
    return play_offset;
}

const uint8_t *codec_service_get_amr_buffer(void)
{
    return amr_output_buffer;
}

const uint8_t *codec_service_get_spi_rx_buffer(void)
{
    return spi_receive_buffer;
}

const uint8_t *codec_service_get_wav_buffer(void)
{
    return codec_workbuf.wav_output_buffer;
}

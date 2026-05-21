#include "codec_service.h"

#include "audio.h"
#include "g711_codec.h"
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
}

void codec_service_start_record(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    wav_total_len = 0;
    play_offset = 44U;
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

#pragma CODE_SECTION(codec_service_encode_recorded, "ramfuncs");
int16_t codec_service_encode_recorded(void)
{
    Uint32 i;

    if (record_count == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    if (record_count > (CODEC_SERVICE_AMR_BUF_SIZE - 1U)) {
        amr_len = 0;
        return CODEC_SERVICE_ERR_ENCODE;
    }

    amr_len = (Uint16)record_count;
    amr_output_buffer[0] = (uint8_t)amr_len;

    for (i = 0U; i < record_count; i++) {
        amr_output_buffer[i + 1U] =
                (uint8_t)(G711A_LinearToAlaw(codec_workbuf.record_buf[i]) & G711_OCTET_MASK);
    }

    return CODEC_SERVICE_OK;
}

int16_t codec_service_spi_exchange_first(void)
{
    spi_send_and_receive((const Uint16 *)amr_output_buffer,
                         (Uint16 *)spi_receive_buffer,
                         CODEC_SERVICE_SPI_PACKET_SIZE);
    return CODEC_SERVICE_OK;
}

int16_t codec_service_spi_exchange_second(void)
{
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

#pragma CODE_SECTION(codec_service_decode_received, "ramfuncs");
int16_t codec_service_decode_received(void)
{
    Uint32 i;
    Uint32 data_length;
    uint8_t *pcm_ptr;
    int16_t pcm_sample;

    if (received_amr_len == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    data_length = (Uint32)received_amr_len * 2UL;
    if ((received_amr_len > CODEC_SERVICE_MAX_RECORD_CNT) ||
        ((data_length + 44UL) > CODEC_SERVICE_WAV_BUF_SIZE)) {
        wav_total_len = 0;
        return CODEC_SERVICE_ERR_DECODE;
    }

    create_wav_header(codec_workbuf.wav_output_buffer, data_length);

    pcm_ptr = &codec_workbuf.wav_output_buffer[44U];
    for (i = 0U; i < received_amr_len; i++) {
        pcm_sample = G711A_AlawToLinear((Uint16)spi_receive_buffer[i + 1U]);
        *pcm_ptr++ = (uint8_t)((Uint16)pcm_sample & 0x00FFU);
        *pcm_ptr++ = (uint8_t)(((Uint16)pcm_sample >> 8U) & 0x00FFU);
    }

    wav_total_len = data_length + 44UL;
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

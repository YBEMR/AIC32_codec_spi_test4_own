#include "codec_service.h"

#include "g711_codec.h"
#include "spi.h"

#pragma DATA_SECTION(pcm_buffer, "ZONE7DATA");
static int16_t pcm_buffer[CODEC_SERVICE_MAX_RECORD_CNT];

#pragma DATA_SECTION(g711_output_buffer, "ZONE7DATA");
static uint8_t g711_output_buffer[CODEC_SERVICE_G711_BUF_SIZE];

#pragma DATA_SECTION(spi_receive_buffer, "ZONE7DATA");
static uint8_t spi_receive_buffer[CODEC_SERVICE_G711_BUF_SIZE];
static Uint32 record_count = 0;
static Uint16 g711_len = 0;
static Uint16 received_g711_len = 0;
static Uint32 pcm_sample_count = 0;
static Uint32 play_sample_index = 0;

void codec_service_reset(void)
{
    record_count = 0;
    g711_len = 0;
    received_g711_len = 0;
    pcm_sample_count = 0;
    play_sample_index = 0;
}

void codec_service_start_record(void)
{
    record_count = 0;
    g711_len = 0;
    received_g711_len = 0;
    pcm_sample_count = 0;
    play_sample_index = 0;
}

int16_t codec_service_record_sample(int16_t sample)
{
    if (record_count >= CODEC_SERVICE_MAX_RECORD_CNT) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    pcm_buffer[record_count++] = sample;
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

    if (record_count > (CODEC_SERVICE_G711_BUF_SIZE - 1U)) {
        g711_len = 0;
        return CODEC_SERVICE_ERR_ENCODE;
    }

    g711_len = (Uint16)record_count;
    g711_output_buffer[0] = (uint8_t)g711_len;

    for (i = 0U; i < record_count; i++) {
        g711_output_buffer[i + 1U] =
                (uint8_t)(G711A_LinearToAlaw(pcm_buffer[i]) & G711_OCTET_MASK);
    }

    return CODEC_SERVICE_OK;
}

int16_t codec_service_spi_exchange_first(void)
{
    spi_send_and_receive((const Uint16 *)g711_output_buffer,
                         (Uint16 *)spi_receive_buffer,
                         CODEC_SERVICE_SPI_PACKET_SIZE);
    return CODEC_SERVICE_OK;
}

int16_t codec_service_spi_exchange_second(void)
{
    spi_send_and_receive((const Uint16 *)g711_output_buffer,
                         (Uint16 *)spi_receive_buffer,
                         CODEC_SERVICE_SPI_PACKET_SIZE);

#if SPI_SLAVE_DATASIZE == SPI_DATASIZE_8BIT
    received_g711_len = (((Uint16)spi_receive_buffer[0] & 0x00ffU) << 8) |
                       (((Uint16)spi_receive_buffer[0] >> 8) & 0x00ffU);
#elif SPI_SLAVE_DATASIZE == SPI_DATASIZE_16BIT
    received_g711_len = (Uint16)spi_receive_buffer[0];
#else
#error "SPI_SLAVE_DATASIZE must be 8BIT or 16BIT"
#endif

    if (received_g711_len != g711_len) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    return CODEC_SERVICE_OK;
}

#pragma CODE_SECTION(codec_service_decode_received, "ramfuncs");
int16_t codec_service_decode_received(void)
{
    Uint32 i;

    if (received_g711_len == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    if (received_g711_len > CODEC_SERVICE_MAX_RECORD_CNT) {
        pcm_sample_count = 0;
        play_sample_index = 0;
        return CODEC_SERVICE_ERR_DECODE;
    }

    for (i = 0U; i < received_g711_len; i++) {
        pcm_buffer[i] = G711A_AlawToLinear((Uint16)spi_receive_buffer[i + 1U]);
    }

    pcm_sample_count = (Uint32)received_g711_len;
    play_sample_index = 0;
    return CODEC_SERVICE_OK;
}

int16_t codec_service_get_play_sample(Uint16 *sample)
{
    if ((sample == 0) || (play_sample_index >= pcm_sample_count)) {
        return CODEC_SERVICE_PLAY_DONE;
    }

    *sample = (Uint16)pcm_buffer[play_sample_index++];

    return CODEC_SERVICE_OK;
}

Uint16 codec_service_get_g711_len(void)
{
    return g711_len;
}

Uint16 codec_service_get_received_g711_len(void)
{
    return received_g711_len;
}

Uint32 codec_service_get_pcm_sample_count(void)
{
    return pcm_sample_count;
}

Uint32 codec_service_get_play_sample_index(void)
{
    return play_sample_index;
}

const uint8_t *codec_service_get_g711_buffer(void)
{
    return g711_output_buffer;
}

const uint8_t *codec_service_get_spi_rx_buffer(void)
{
    return spi_receive_buffer;
}

const int16_t *codec_service_get_pcm_buffer(void)
{
    return pcm_buffer;
}

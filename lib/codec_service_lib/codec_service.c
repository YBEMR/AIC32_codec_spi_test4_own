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

static int16 stream_capture_frame[CODEC_SERVICE_STREAM_FRAME_SAMPLES];
static Uint16 stream_capture_index = 0;
static Uint16 stream_capture_active = 0;

static int16 stream_pcm_frames[CODEC_SERVICE_STREAM_PCM_FRAME_CAPACITY]
                              [CODEC_SERVICE_STREAM_FRAME_SAMPLES];
static Uint16 stream_pcm_read_index = 0;
static Uint16 stream_pcm_write_index = 0;
static Uint16 stream_pcm_frame_count = 0;

static Uint16 stream_encoded_frames[CODEC_SERVICE_STREAM_ENCODED_FRAME_CAPACITY]
                                  [CODEC_SERVICE_STREAM_FRAME_OCTETS];
static Uint16 stream_encoded_read_index = 0;
static Uint16 stream_encoded_write_index = 0;
static Uint16 stream_encoded_frame_count = 0;

static int16 stream_play_frames[CODEC_SERVICE_STREAM_PLAY_FRAME_CAPACITY]
                               [CODEC_SERVICE_STREAM_FRAME_SAMPLES];
static Uint16 stream_play_read_index = 0;
static Uint16 stream_play_write_index = 0;
static Uint16 stream_play_frame_count = 0;
static Uint16 stream_play_sample_index = 0;

static Uint32 stream_overflow_count = 0;
static Uint32 stream_underflow_count = 0;

static Uint16 codec_service_stream_next_encoded_index(Uint16 index)
{
    index++;
    if (index >= CODEC_SERVICE_STREAM_ENCODED_FRAME_CAPACITY) {
        index = 0;
    }

    return index;
}

static Uint16 codec_service_stream_next_pcm_index(Uint16 index)
{
    index++;
    if (index >= CODEC_SERVICE_STREAM_PCM_FRAME_CAPACITY) {
        index = 0;
    }

    return index;
}

static Uint16 codec_service_stream_next_play_index(Uint16 index)
{
    index++;
    if (index >= CODEC_SERVICE_STREAM_PLAY_FRAME_CAPACITY) {
        index = 0;
    }

    return index;
}

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

void codec_service_stream_reset(void)
{
    stream_capture_index = 0;
    stream_capture_active = 0;

    stream_pcm_read_index = 0;
    stream_pcm_write_index = 0;
    stream_pcm_frame_count = 0;

    stream_encoded_read_index = 0;
    stream_encoded_write_index = 0;
    stream_encoded_frame_count = 0;

    stream_play_read_index = 0;
    stream_play_write_index = 0;
    stream_play_frame_count = 0;
    stream_play_sample_index = 0;

    stream_overflow_count = 0;
    stream_underflow_count = 0;
}

void codec_service_stream_start_capture(void)
{
    stream_capture_index = 0;
    stream_capture_active = 1;
}

void codec_service_stream_stop_capture(void)
{
    stream_capture_active = 0;
    stream_capture_index = 0;
}

int16_t codec_service_stream_record_sample(int16_t sample)
{
    Uint16 i;

    if (stream_capture_active == 0U) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    stream_capture_frame[stream_capture_index++] = (int16)sample;

    if (stream_capture_index < CODEC_SERVICE_STREAM_FRAME_SAMPLES) {
        return CODEC_SERVICE_OK;
    }

    stream_capture_index = 0;

    if (stream_pcm_frame_count >= CODEC_SERVICE_STREAM_PCM_FRAME_CAPACITY) {
        stream_overflow_count++;
        return CODEC_SERVICE_ERR_OVERFLOW;
    }

    for (i = 0U; i < CODEC_SERVICE_STREAM_FRAME_SAMPLES; i++) {
        stream_pcm_frames[stream_pcm_write_index][i] = stream_capture_frame[i];
    }

    stream_pcm_write_index =
            codec_service_stream_next_pcm_index(stream_pcm_write_index);
    stream_pcm_frame_count++;

    return CODEC_SERVICE_FRAME_READY;
}

Uint16 codec_service_stream_has_pcm_frame(void)
{
    return (stream_pcm_frame_count > 0U) ? 1U : 0U;
}

int16_t codec_service_stream_process_encode(void)
{
    if (stream_pcm_frame_count == 0U) {
        stream_underflow_count++;
        return CODEC_SERVICE_ERR_UNDERFLOW;
    }

    if (stream_encoded_frame_count >=
            CODEC_SERVICE_STREAM_ENCODED_FRAME_CAPACITY) {
        stream_overflow_count++;
        return CODEC_SERVICE_ERR_OVERFLOW;
    }

    G711A_EncodeFrame(stream_pcm_frames[stream_pcm_read_index],
                      stream_encoded_frames[stream_encoded_write_index],
                      CODEC_SERVICE_STREAM_FRAME_SAMPLES);

    stream_pcm_read_index =
            codec_service_stream_next_pcm_index(stream_pcm_read_index);
    stream_pcm_frame_count--;

    stream_encoded_write_index =
            codec_service_stream_next_encoded_index(stream_encoded_write_index);
    stream_encoded_frame_count++;

    return CODEC_SERVICE_OK;
}

Uint16 codec_service_stream_has_encoded_frame(void)
{
    return (stream_encoded_frame_count > 0U) ? 1U : 0U;
}

int16_t codec_service_stream_get_encoded_frame(Uint16 *frame_words,
                                               Uint16 max_words,
                                               Uint16 *out_words)
{
    Uint16 i;

    if ((frame_words == 0) || (out_words == 0)) {
        return CODEC_SERVICE_ERR_PARAM;
    }

    if (max_words < CODEC_SERVICE_STREAM_FRAME_OCTETS) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    if (stream_encoded_frame_count == 0U) {
        *out_words = 0;
        stream_underflow_count++;
        return CODEC_SERVICE_ERR_UNDERFLOW;
    }

    for (i = 0U; i < CODEC_SERVICE_STREAM_FRAME_OCTETS; i++) {
        frame_words[i] =
                stream_encoded_frames[stream_encoded_read_index][i] &
                G711_OCTET_MASK;
    }

    *out_words = CODEC_SERVICE_STREAM_FRAME_OCTETS;
    stream_encoded_read_index =
            codec_service_stream_next_encoded_index(stream_encoded_read_index);
    stream_encoded_frame_count--;

    return CODEC_SERVICE_OK;
}

int16_t codec_service_stream_put_play_frame(const Uint16 *g711_words,
                                            Uint16 octets)
{
    if (g711_words == 0) {
        return CODEC_SERVICE_ERR_PARAM;
    }

    if (octets != CODEC_SERVICE_STREAM_FRAME_OCTETS) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    if (stream_play_frame_count >= CODEC_SERVICE_STREAM_PLAY_FRAME_CAPACITY) {
        stream_overflow_count++;
        return CODEC_SERVICE_ERR_OVERFLOW;
    }

    G711A_DecodeFrame(g711_words,
                      stream_play_frames[stream_play_write_index],
                      CODEC_SERVICE_STREAM_FRAME_OCTETS);

    stream_play_write_index =
            codec_service_stream_next_play_index(stream_play_write_index);
    stream_play_frame_count++;

    return CODEC_SERVICE_OK;
}

int16_t codec_service_stream_get_play_sample(Uint16 *sample)
{
    if (sample == 0) {
        return CODEC_SERVICE_ERR_PARAM;
    }

    if (stream_play_frame_count == 0U) {
        stream_underflow_count++;
        return CODEC_SERVICE_PLAY_DONE;
    }

    *sample = (Uint16)stream_play_frames[stream_play_read_index]
                                     [stream_play_sample_index++];

    if (stream_play_sample_index >= CODEC_SERVICE_STREAM_FRAME_SAMPLES) {
        stream_play_sample_index = 0;
        stream_play_read_index =
                codec_service_stream_next_play_index(stream_play_read_index);
        stream_play_frame_count--;
    }

    return CODEC_SERVICE_OK;
}

Uint32 codec_service_stream_get_pcm_frame_count(void)
{
    return (Uint32)stream_pcm_frame_count;
}

Uint32 codec_service_stream_get_encoded_frame_count(void)
{
    return (Uint32)stream_encoded_frame_count;
}

Uint32 codec_service_stream_get_play_frame_count(void)
{
    return (Uint32)stream_play_frame_count;
}

Uint32 codec_service_stream_get_overflow_count(void)
{
    return stream_overflow_count;
}

Uint32 codec_service_stream_get_underflow_count(void)
{
    return stream_underflow_count;
}

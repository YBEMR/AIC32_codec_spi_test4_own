#include "codec_service_internal.h"

#include "audio.h"
#include "spi.h"
#include <string.h>

#pragma DATA_SECTION(codec_workbuf, "ZONE7DATA");
codec_service_workbuf_t codec_workbuf;

#pragma DATA_SECTION(amr_output_buffer, "ZONE7DATA");
uint8_t amr_output_buffer[CODEC_SERVICE_AMR_BUF_SIZE];

#pragma DATA_SECTION(spi_receive_buffer, "ZONE7DATA");
uint8_t spi_receive_buffer[CODEC_SERVICE_AMR_BUF_SIZE];

Uint32 record_count = 0;
Uint16 amr_len = 0;
Uint16 received_amr_len = 0;
Uint32 play_sample_count = 0;
Uint32 play_sample_offset = 0;
static int16_t codec_service_decode_pcm_frame[AMR_PCM_FRAME_SAMPLES];

/**
 * @brief 复位 codec_service 内部缓存和播放状态。
 *
 * 该函数会清空录音、AMR 长度和 PCM16 播放状态，避免上一次流程残留。
 *
 * @return void
 */
void codec_service_reset(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    play_sample_count = 0;
    play_sample_offset = 0;
    codec_service_ptt_frame_reset_state();
    codec_service_ptt_spi_reset_state();
}

/**
 * @brief 开始新一段录音并清空相关播放状态。
 *
 * 录音开始后，AMR 长度和 PCM16 播放位置都需要重新归零。
 *
 * @return void
 */
void codec_service_start_record(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    play_sample_count = 0;
    play_sample_offset = 0;
    codec_service_ptt_frame_reset_state();
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
    int16_t result;

    if (record_count == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

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
/**
 * @brief 将收到的整段 AMR 数据解码到 PCM16 播放缓存。
 *
 * 当前第一版仍然按整段 AMR 收包。AMR 数据包含 IETF 文件头，
 * 本函数跳过文件头后逐帧解析长度，并把每帧解码出的 160 个 sample
 * 直接追加到 play_buf，不再生成 WAV 头或 WAV byte buffer。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，负值表示失败。
 */
int16_t codec_service_decode_received(void)
{
    const uint8_t *amr_data;
    Uint16 offset;
    uint16_t frame_len;
    uint16_t pcm_sample_count;
    Uint16 i;

    if (received_amr_len == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    amr_data = &spi_receive_buffer[1];
    if ((received_amr_len < AMR_IETF_HDR_LEN) ||
        (memcmp(amr_data, amr_file_header_magic, AMR_IETF_HDR_LEN) != 0)) {
        play_sample_count = 0;
        play_sample_offset = 0;
        return CODEC_SERVICE_ERR_DECODE;
    }

    if (amr_decode_frame_reset() != 0) {
        play_sample_count = 0;
        play_sample_offset = 0;
        return CODEC_SERVICE_ERR_DECODE;
    }

    play_sample_count = 0;
    play_sample_offset = 0;
    offset = AMR_IETF_HDR_LEN;

    while (offset < received_amr_len) {
        if (amr_ietf_frame_length(amr_data[offset], &frame_len) != 0) {
            play_sample_count = 0;
            return CODEC_SERVICE_ERR_DECODE;
        }

        if ((frame_len == 0U) || ((Uint32)offset + frame_len > received_amr_len)) {
            play_sample_count = 0;
            return CODEC_SERVICE_ERR_LENGTH;
        }

        if (amr_decode_pcm16_frame(&amr_data[offset],
                                   frame_len,
                                   codec_service_decode_pcm_frame,
                                   AMR_PCM_FRAME_SAMPLES,
                                   &pcm_sample_count) != 0) {
            play_sample_count = 0;
            return CODEC_SERVICE_ERR_DECODE;
        }

        if ((play_sample_count + pcm_sample_count) > CODEC_SERVICE_MAX_RECORD_CNT) {
            play_sample_count = 0;
            return CODEC_SERVICE_ERR_LENGTH;
        }

        for (i = 0; i < pcm_sample_count; i++) {
            codec_workbuf.play_buf[play_sample_count++] = codec_service_decode_pcm_frame[i];
        }

        offset = (Uint16)(offset + frame_len);
    }

    return CODEC_SERVICE_OK;
}

/**
 * @brief 获取下一点播放 sample。
 *
 * 播放路径已经统一为 PCM16 缓存，因此这里直接从 play_buf 取 16-bit sample，
 * 不再从 WAV byte buffer 合成 sample。
 *
 * @param sample 输出参数，返回下一点 16-bit PCM sample。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，CODEC_SERVICE_PLAY_DONE 表示没有更多 sample。
 */
int16_t codec_service_get_play_sample(Uint16 *sample)
{
    if (sample == 0) {
        return CODEC_SERVICE_PLAY_DONE;
    }

    if (play_sample_offset >= play_sample_count) {
        return CODEC_SERVICE_PLAY_DONE;
    }

    *sample = (Uint16)codec_workbuf.play_buf[play_sample_offset++];
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

/**
 * @brief 获取当前可播放 PCM16 sample 数。
 *
 * 该值用于主状态机打印和调试，单位是 sample，不包含 WAV 头或 byte 转换。
 *
 * @return Uint32 当前播放缓存中的 PCM16 sample 数。
 */
Uint32 codec_service_get_play_sample_count(void)
{
    return play_sample_count;
}

const uint8_t *codec_service_get_amr_buffer(void)
{
    return amr_output_buffer;
}

const uint8_t *codec_service_get_spi_rx_buffer(void)
{
    return spi_receive_buffer;
}


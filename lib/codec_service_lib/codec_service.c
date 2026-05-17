#include "codec_service.h"

#include "audio.h"
#include "spi.h"

typedef union {
    int16_t record_buf[CODEC_SERVICE_MAX_RECORD_CNT];
    /* PTT 新路径直接保存 PCM16 sample；与录音/WAV 缓冲复用同一块 Zone7 大内存。 */
    int16_t ptt_play_buf[CODEC_SERVICE_MAX_RECORD_CNT];
    uint8_t wav_output_buffer[CODEC_SERVICE_WAV_BUF_SIZE];
} codec_service_workbuf_t;

typedef enum {
    CODEC_SERVICE_PLAY_SOURCE_WAV = 0,
    CODEC_SERVICE_PLAY_SOURCE_PTT_PCM = 1
} codec_service_play_source_t;

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
static codec_service_play_source_t play_source = CODEC_SERVICE_PLAY_SOURCE_WAV;
static Uint32 ptt_encode_sample_offset = 0;
static Uint16 ptt_encode_frame_id = 0;
static Uint16 ptt_encode_frame_count = 0;
static Uint32 ptt_play_sample_count = 0;
static Uint32 ptt_play_sample_offset = 0;
static int16_t ptt_pcm_tail[AMR_PCM_FRAME_SAMPLES];
static int16_t ptt_pcm_decode_frame[AMR_PCM_FRAME_SAMPLES];

/**
 * @brief 复位 codec_service 内部缓存和播放状态。
 *
 * 该函数会同时清空旧 WAV 播放状态和新 PTT PCM16 播放状态，避免上一次播放源残留。
 *
 * @return void
 */
void codec_service_reset(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    wav_total_len = 0;
    play_offset = 44U;
    play_source = CODEC_SERVICE_PLAY_SOURCE_WAV;
    ptt_encode_sample_offset = 0;
    ptt_encode_frame_id = 0;
    ptt_encode_frame_count = 0;
    ptt_play_sample_count = 0;
    ptt_play_sample_offset = 0;
}

/**
 * @brief 开始新一段录音并清空相关播放状态。
 *
 * 录音开始后，旧的 AMR/WAV 长度和 PTT PCM16 播放位置都需要重新归零。
 *
 * @return void
 */
void codec_service_start_record(void)
{
    record_count = 0;
    amr_len = 0;
    received_amr_len = 0;
    wav_total_len = 0;
    play_offset = 44U;
    play_source = CODEC_SERVICE_PLAY_SOURCE_WAV;
    ptt_encode_sample_offset = 0;
    ptt_encode_frame_id = 0;
    ptt_encode_frame_count = 0;
    ptt_play_sample_count = 0;
    ptt_play_sample_offset = 0;
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
 * @brief 解码旧整段 AMR 数据到 WAV 缓冲区。
 *
 * 该函数保留原有测试流程：解码结果仍写入 wav_output_buffer，并把播放源切回 WAV。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，负值表示失败。
 */
int16_t codec_service_decode_received(void)
{
    int16_t result;

    if (received_amr_len == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

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
    play_source = CODEC_SERVICE_PLAY_SOURCE_WAV;
    return CODEC_SERVICE_OK;
}

/**
 * @brief 获取下一点播放 sample。
 *
 * WAV 播放源沿用原逻辑，从 WAV data 区两个 byte 合成为 16-bit sample；
 * PTT PCM 播放源直接从 ptt_play_buf 读取 16-bit sample，避免新 PTT 路径再经过 WAV byte buffer。
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

    if (play_source == CODEC_SERVICE_PLAY_SOURCE_PTT_PCM) {
        if (ptt_play_sample_offset >= ptt_play_sample_count) {
            return CODEC_SERVICE_PLAY_DONE;
        }

        *sample = (Uint16)codec_workbuf.ptt_play_buf[ptt_play_sample_offset++];
        return CODEC_SERVICE_OK;
    }

    if ((wav_total_len <= 44U) || (play_offset + 1U >= wav_total_len)) {
        return CODEC_SERVICE_PLAY_DONE;
    }

    *sample = (Uint16)codec_workbuf.wav_output_buffer[play_offset] |
              ((Uint16)codec_workbuf.wav_output_buffer[play_offset + 1U] << 8);
    play_offset += 2U;

    return CODEC_SERVICE_OK;
}

/**
 * @brief 开始 PTT 逐帧编码流程。
 *
 * 该函数根据当前录音 sample 数计算总帧数，并重置 AMR 编码器状态。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，负值表示失败。
 */
int16_t codec_service_ptt_encode_begin(void)
{
    if (record_count == 0) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    ptt_encode_sample_offset = 0;
    ptt_encode_frame_id = 0;
    // 向上取整计算总帧数
    ptt_encode_frame_count = (Uint16)((record_count + AMR_PCM_FRAME_SAMPLES - 1U) /
                                      AMR_PCM_FRAME_SAMPLES);
    amr_len = 0;

    if (amr_encode_frame_reset() != 0) {
        ptt_encode_frame_count = 0;
        return CODEC_SERVICE_ERR_ENCODE;
    }

    return CODEC_SERVICE_OK;
}

/**
 * @brief 从录音缓存中编码下一帧 PTT AMR 数据。
 *
 * 每次调用输出一帧 raw IETF AMR frame，不包含 .amr 文件头；最后不足
 * 160 sample 的录音尾帧会在内部补零。
 *
 * @param amr_frame 输出缓冲区，用于保存编码后的 AMR frame。
 * @param amr_frame_buf_size 输出缓冲区大小，单位为 byte。
 * @param amr_frame_len 输出参数，返回实际 AMR frame 长度，单位为 byte。
 * @param frame_id 输出参数，返回当前帧序号，从 0 开始。
 * @param frame_count 输出参数，返回本段 PTT 语音总帧数。
 * @param is_last 输出参数，非 0 表示当前帧是本段最后一帧。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，CODEC_SERVICE_PTT_DONE 表示已无帧可取，负值表示失败。
 */
int16_t codec_service_ptt_encode_next(uint8_t *amr_frame,
                                      Uint16 amr_frame_buf_size,
                                      Uint16 *amr_frame_len,
                                      Uint16 *frame_id,
                                      Uint16 *frame_count,
                                      Uint16 *is_last)
{
    const int16_t *pcm_frame;
    Uint32 remain;
    Uint16 i;
    int16_t result;

    if ((amr_frame == 0) || (amr_frame_len == 0) ||
        (frame_id == 0) || (frame_count == 0) || (is_last == 0)) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    if (ptt_encode_frame_id >= ptt_encode_frame_count) {
        *amr_frame_len = 0;
        return CODEC_SERVICE_PTT_DONE;
    }

    remain = record_count - ptt_encode_sample_offset;
    if (remain >= AMR_PCM_FRAME_SAMPLES) {
        pcm_frame = &codec_workbuf.record_buf[ptt_encode_sample_offset];
        ptt_encode_sample_offset += AMR_PCM_FRAME_SAMPLES;
    } else {
        for (i = 0; i < (Uint16)remain; i++) {
            ptt_pcm_tail[i] = codec_workbuf.record_buf[ptt_encode_sample_offset + i];
        }
        while (i < AMR_PCM_FRAME_SAMPLES) {
            ptt_pcm_tail[i++] = 0;
        }
        pcm_frame = ptt_pcm_tail;
        ptt_encode_sample_offset = record_count;
    }

    result = amr_encode_pcm16_frame(pcm_frame,
                                    amr_frame,
                                    amr_frame_buf_size,
                                    amr_frame_len);
    if (result != 0) {
        *amr_frame_len = 0;
        return CODEC_SERVICE_ERR_ENCODE;
    }

    *frame_id = ptt_encode_frame_id;
    *frame_count = ptt_encode_frame_count;
    *is_last = (Uint16)((ptt_encode_frame_id + 1U) >= ptt_encode_frame_count);
    // 更新要编码的下一帧数据的序号，如果是最后一帧，下次调用会判断到并返回PPT_DONE
    ptt_encode_frame_id++;

    return CODEC_SERVICE_OK;
}

/**
 * @brief 开始 PTT 逐帧解码流程。
 *
 * 该函数会清空 PTT PCM16 播放缓存状态，并重置 AMR 解码器状态。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，负值表示失败。
 */
int16_t codec_service_ptt_decode_begin(void)
{
    wav_total_len = 0;
    play_offset = 44U;
    play_source = CODEC_SERVICE_PLAY_SOURCE_PTT_PCM;
    ptt_play_sample_count = 0;
    ptt_play_sample_offset = 0;
    received_amr_len = 0;

    if (amr_decode_frame_reset() != 0) {
        return CODEC_SERVICE_ERR_DECODE;
    }

    return CODEC_SERVICE_OK;
}

/**
 * @brief 解码一帧 PTT AMR 数据并追加到 PCM16 播放缓存。
 *
 * 输入数据是一帧 raw IETF AMR frame，不包含 .amr 文件头。
 *
 * @param amr_frame 输入 AMR frame 数据。
 * @param amr_frame_len 输入 AMR frame 长度，单位为 byte。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，负值表示失败。
 */
int16_t codec_service_ptt_decode_frame(const uint8_t *amr_frame,
                                       Uint16 amr_frame_len)
{
    uint16_t pcm_sample_count;
    Uint16 i;

    if ((amr_frame == 0) || (amr_frame_len == 0)) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    if (amr_decode_pcm16_frame(amr_frame,
                               amr_frame_len,
                               ptt_pcm_decode_frame,
                               AMR_PCM_FRAME_SAMPLES,
                               &pcm_sample_count) != 0) {
        return CODEC_SERVICE_ERR_DECODE;
    }

    if ((ptt_play_sample_count + pcm_sample_count) > CODEC_SERVICE_MAX_RECORD_CNT) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    for (i = 0; i < pcm_sample_count; i++) {
        codec_workbuf.ptt_play_buf[ptt_play_sample_count++] = ptt_pcm_decode_frame[i];
    }

    received_amr_len += amr_frame_len;
    return CODEC_SERVICE_OK;
}

/**
 * @brief 结束 PTT 逐帧解码流程并准备 PCM16 播放。
 *
 * 调用后，codec_service_get_play_sample() 会直接从 PTT PCM16 缓存取样播放。
 *
 * @return int16_t CODEC_SERVICE_OK 表示成功，负值表示失败。
 */
int16_t codec_service_ptt_decode_finish(void)
{
    play_source = CODEC_SERVICE_PLAY_SOURCE_PTT_PCM;
    ptt_play_sample_offset = 0;
    wav_total_len = 0;
    play_offset = 44U;
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
 * @brief 获取当前可播放数据长度。
 *
 * 旧 WAV 流程返回 WAV 文件总 byte 数；PTT PCM 流程没有 WAV 头，因此返回 PCM16 byte 数。
 *
 * @return Uint32 当前播放数据长度，单位为 byte。
 */
Uint32 codec_service_get_wav_len(void)
{
    if (play_source == CODEC_SERVICE_PLAY_SOURCE_PTT_PCM) {
        return ptt_play_sample_count * 2UL;
    }

    return wav_total_len;
}

/**
 * @brief 获取当前播放偏移。
 *
 * 旧 WAV 流程返回 WAV byte 偏移；PTT PCM 流程返回 PCM16 sample 偏移。
 *
 * @return Uint32 当前播放偏移，单位由播放源决定。
 */
Uint32 codec_service_get_play_offset(void)
{
    if (play_source == CODEC_SERVICE_PLAY_SOURCE_PTT_PCM) {
        return ptt_play_sample_offset;
    }

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

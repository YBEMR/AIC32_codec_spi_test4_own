#include "codec_service_internal.h"

#include "audio.h"

static Uint32 ptt_encode_sample_offset = 0;
static Uint16 ptt_encode_frame_id = 0;
static Uint16 ptt_encode_frame_count = 0;
static int16_t ptt_pcm_tail[AMR_PCM_FRAME_SAMPLES];
static int16_t ptt_pcm_decode_frame[AMR_PCM_FRAME_SAMPLES];

/**
 * @brief 复位 PTT 单帧编解码模块内部状态。
 *
 * 该函数只清空逐帧编码游标和帧计数，不触碰 AMR core state；真正的
 * encoder/decoder reset 仍由 begin 接口执行。
 *
 * @return void
 */
void codec_service_ptt_frame_reset_state(void)
{
    ptt_encode_sample_offset = 0;
    ptt_encode_frame_id = 0;
    ptt_encode_frame_count = 0;
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
    play_sample_count = 0;
    play_sample_offset = 0;
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

    if ((play_sample_count + pcm_sample_count) > CODEC_SERVICE_MAX_RECORD_CNT) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    for (i = 0; i < pcm_sample_count; i++) {
        codec_workbuf.play_buf[play_sample_count++] = ptt_pcm_decode_frame[i];
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
    play_sample_offset = 0;
    return CODEC_SERVICE_OK;
}

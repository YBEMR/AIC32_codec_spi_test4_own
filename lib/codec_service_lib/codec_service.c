#include "codec_service.h"

#include "audio.h"
#include "spi.h"
#include <string.h>

typedef union {
    int16_t record_buf[CODEC_SERVICE_MAX_RECORD_CNT];
    /* 播放路径统一保存 PCM16 sample；与录音缓存复用同一块 Zone7 大内存。 */
    int16_t play_buf[CODEC_SERVICE_MAX_RECORD_CNT];
} codec_service_workbuf_t;

#pragma DATA_SECTION(codec_workbuf, "ZONE7DATA");
static codec_service_workbuf_t codec_workbuf;

#pragma DATA_SECTION(amr_output_buffer, "ZONE7DATA");
static uint8_t amr_output_buffer[CODEC_SERVICE_AMR_BUF_SIZE];

#pragma DATA_SECTION(spi_receive_buffer, "ZONE7DATA");
static uint8_t spi_receive_buffer[CODEC_SERVICE_AMR_BUF_SIZE];

#pragma DATA_SECTION(ptt_spi_tx_words, "ZONE7DATA");
static Uint16 ptt_spi_tx_words[CODEC_SERVICE_SPI_PACKET_SIZE];

#pragma DATA_SECTION(ptt_spi_rx_words, "ZONE7DATA");
static Uint16 ptt_spi_rx_words[CODEC_SERVICE_SPI_PACKET_SIZE];

static Uint32 record_count = 0;
static Uint16 amr_len = 0;
static Uint16 received_amr_len = 0;
static Uint16 ptt_spi_session_id = 0;
static Uint32 ptt_encode_sample_offset = 0;
static Uint16 ptt_encode_frame_id = 0;
static Uint16 ptt_encode_frame_count = 0;
static Uint32 play_sample_count = 0;
static Uint32 play_sample_offset = 0;
static int16_t ptt_pcm_tail[AMR_PCM_FRAME_SAMPLES];
static int16_t ptt_pcm_decode_frame[AMR_PCM_FRAME_SAMPLES];

#define CODEC_SERVICE_PTT_SPI_BLOCK_MAGIC          0x5054U
#define CODEC_SERVICE_PTT_SPI_BLOCK_VERSION        0x0001U
#define CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS   8U
#define CODEC_SERVICE_PTT_SPI_READY_TIMEOUT_LOOP   5000000UL
#define CODEC_SERVICE_PTT_SPI_DONE_TIMEOUT_LOOP    5000000UL
#define CODEC_SERVICE_PTT_DSP_REQ_GUARD_LOOP       20000UL

#define CODEC_SERVICE_PTT_SPI_BLOCK_DSP_UPLOAD_BLOCK      1U
#define CODEC_SERVICE_PTT_SPI_BLOCK_DSP_DOWNLOAD_REQ      2U
#define CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_DOWNLOAD_BLOCK  3U
#define CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_NO_SESSION      4U
#define CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_STATUS          5U
#define CODEC_SERVICE_PTT_SPI_BLOCK_DSP_STATUS_POLL       6U

#define CODEC_SERVICE_PTT_SPI_STATUS_OK                   0U
#define CODEC_SERVICE_PTT_SPI_PAYLOAD_WORD0               CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS
#define CODEC_SERVICE_PTT_SPI_MAX_PAYLOAD_BYTES \
    ((CODEC_SERVICE_SPI_PACKET_SIZE - CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS) * 2U)

/**
 * @brief 在 DSP_REQ 拉低后插入一小段保护延时。
 *
 * 该延时让 Art-Pi 的轮询线程有机会稳定采样到 DSP_REQ 低电平，避免 DSP
 * 很快再次拉高请求线时，Art-Pi 把下一次请求误认为上一次请求尚未释放。
 *
 * @param loop_count 软件空循环次数。
 *
 * @return void
 */
static void codec_service_ptt_spi_guard_delay(Uint32 loop_count)
{
    volatile Uint32 i;

    for (i = 0; i < loop_count; i++) {
    }
}

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
    ptt_encode_sample_offset = 0;
    ptt_encode_frame_id = 0;
    ptt_encode_frame_count = 0;
    ptt_spi_session_id = 0;
    play_sample_count = 0;
    play_sample_offset = 0;
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
    ptt_encode_sample_offset = 0;
    ptt_encode_frame_id = 0;
    ptt_encode_frame_count = 0;
    play_sample_count = 0;
    play_sample_offset = 0;
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

/**
 * @brief 通过 PTT 三线握手执行一次 SPI block 传输。
 *
 * DSP 先拉高 DSP_REQ，请求 Art-Pi arm SPI slave DMA；检测到 SPI_READY
 * 后再输出 SPI clock。无论传输是否成功，函数退出前都会拉低 DSP_REQ。
 *
 * @return int16_t CODEC_SERVICE_OK 表示 SPI 传输完成，负值表示等待超时。
 */
static int16_t codec_service_ptt_spi_transfer_block(void)
{
    if (spi_ptt_wait_spi_not_ready(CODEC_SERVICE_PTT_SPI_DONE_TIMEOUT_LOOP) != SPI_PTT_OK) {
        return CODEC_SERVICE_ERR_TIMEOUT;
    }

    spi_ptt_set_dsp_req(1U);
    if (spi_ptt_wait_spi_ready(CODEC_SERVICE_PTT_SPI_READY_TIMEOUT_LOOP) != SPI_PTT_OK) {
        spi_ptt_set_dsp_req(0U);
        codec_service_ptt_spi_guard_delay(CODEC_SERVICE_PTT_DSP_REQ_GUARD_LOOP);
        return CODEC_SERVICE_ERR_TIMEOUT;
    }

    spi_send_and_receive(ptt_spi_tx_words, ptt_spi_rx_words, CODEC_SERVICE_SPI_PACKET_SIZE);
    spi_ptt_set_dsp_req(0U);
    if (spi_ptt_wait_spi_not_ready(CODEC_SERVICE_PTT_SPI_DONE_TIMEOUT_LOOP) != SPI_PTT_OK) {
        return CODEC_SERVICE_ERR_TIMEOUT;
    }
    codec_service_ptt_spi_guard_delay(CODEC_SERVICE_PTT_DSP_REQ_GUARD_LOOP);

    return CODEC_SERVICE_OK;
}

/**
 * @brief 清空 PTT SPI tx/rx word 缓冲区。
 *
 * PTT block 只用 header 中的 payload_bytes 表示逻辑有效载荷长度，但底层
 * SPI DMA 仍按固定 word 数交换，因此发送前要把未使用区域清零，方便调试。
 *
 * @return void
 */
static void codec_service_ptt_spi_clear_words(void)
{
    memset(ptt_spi_tx_words, 0, sizeof(ptt_spi_tx_words));
    memset(ptt_spi_rx_words, 0, sizeof(ptt_spi_rx_words));
}

/**
 * @brief 填写 PTT SPI block 的公共 8-word 头。
 *
 * 上传 block 中 word4..word7 分别表示 session_id、block_id、
 * block_count 和 payload_bytes；status poll 中复用 word4 表示 session_id。
 *
 * @param block_type block 类型。
 * @param session_id PTT session 编号。
 * @param word5 第 5 个 word 的调用方语义。
 * @param word6 第 6 个 word 的调用方语义。
 * @param word7 第 7 个 word 的调用方语义。
 *
 * @return void
 */
static void codec_service_ptt_spi_fill_header(Uint16 block_type,
                                              Uint16 session_id,
                                              Uint16 word5,
                                              Uint16 word6,
                                              Uint16 word7)
{
    ptt_spi_tx_words[0] = CODEC_SERVICE_PTT_SPI_BLOCK_MAGIC;
    ptt_spi_tx_words[1] = CODEC_SERVICE_PTT_SPI_BLOCK_VERSION;
    ptt_spi_tx_words[2] = CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS;
    ptt_spi_tx_words[3] = block_type;
    ptt_spi_tx_words[4] = session_id;
    ptt_spi_tx_words[5] = word5;
    ptt_spi_tx_words[6] = word6;
    ptt_spi_tx_words[7] = word7;
}

/**
 * @brief 将 AMR byte payload 打包为 SPI 16-bit word。
 *
 * 每个 word 的高字节放 payload 的前一个 byte，低字节放后一个 byte；
 * payload 为奇数字节时，最后一个 word 的低字节补 0。
 *
 * @param payload 输入 AMR byte 数据。
 * @param payload_bytes 输入 byte 数。
 *
 * @return void
 */
static void codec_service_ptt_spi_pack_payload(const uint8_t *payload, Uint16 payload_bytes)
{
    Uint16 i;
    Uint16 word_count;
    Uint16 hi;
    Uint16 lo;

    word_count = (Uint16)((payload_bytes + 1U) / 2U);
    for (i = 0; i < word_count; i++) {
        hi = (Uint16)payload[i * 2U];
        if (((i * 2U) + 1U) < payload_bytes) {
            lo = (Uint16)payload[(i * 2U) + 1U];
        } else {
            lo = 0U;
        }
        ptt_spi_tx_words[CODEC_SERVICE_PTT_SPI_PAYLOAD_WORD0 + i] =
            (Uint16)((hi << 8) | lo);
    }
}

/**
 * @brief 校验 Art-Pi 返回的 PTT status block。
 *
 * DSP 上传当前 block 后，会再发一次 DSP_STATUS_POLL。由于 Art-Pi 的
 * SPI tx buffer 是在本次传输前预装的，所以 poll 收到的是上一块上传结果。
 *
 * @param session_id 期望的 session 编号。
 * @param block_id 期望确认的 block 编号。
 *
 * @return int16_t CODEC_SERVICE_OK 表示 ACK 正确，负值表示状态错误。
 */
static int16_t codec_service_ptt_spi_check_status(Uint16 session_id, Uint16 block_id)
{
    if ((ptt_spi_rx_words[0] != CODEC_SERVICE_PTT_SPI_BLOCK_MAGIC) ||
        (ptt_spi_rx_words[1] != CODEC_SERVICE_PTT_SPI_BLOCK_VERSION) ||
        (ptt_spi_rx_words[2] != CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS) ||
        (ptt_spi_rx_words[3] != CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_STATUS)) {
        return CODEC_SERVICE_ERR_DECODE;
    }

    if ((ptt_spi_rx_words[4] != session_id) ||
        (ptt_spi_rx_words[6] != block_id) ||
        (ptt_spi_rx_words[7] != CODEC_SERVICE_PTT_SPI_STATUS_OK)) {
        return CODEC_SERVICE_ERR_REMOTE;
    }

    return CODEC_SERVICE_OK;
}

/**
 * @brief 执行一次 PTT SPI block 状态探针。
 *
 * 该函数用于验证 DSP_REQ -> SPI_READY -> SPI clock 的三线握手顺序。
 * DSP 先拉高 DSP_REQ，请求 Art-Pi arm SPI slave DMA；等 SPI_READY 为高后，
 * 发送一个最小 DSP_DOWNLOAD_REQ block，并期望收到 Art-Pi 的 ARTPI_STATUS
 * block。传输结束后无论成功失败都会拉低 DSP_REQ。
 *
 * @return int16_t CODEC_SERVICE_OK 表示收到合法 ARTPI_STATUS，负值表示超时或 block 头不合法。
 */
int16_t codec_service_ptt_spi_status_probe(void)
{
    int16_t result;

    codec_service_ptt_spi_clear_words();
    codec_service_ptt_spi_fill_header(CODEC_SERVICE_PTT_SPI_BLOCK_DSP_DOWNLOAD_REQ,
                                      0U,
                                      0U,
                                      0U,
                                      0U);

    result = codec_service_ptt_spi_transfer_block();
    if (result != CODEC_SERVICE_OK) {
        return result;
    }

    if ((ptt_spi_rx_words[0] != CODEC_SERVICE_PTT_SPI_BLOCK_MAGIC) ||
        (ptt_spi_rx_words[1] != CODEC_SERVICE_PTT_SPI_BLOCK_VERSION) ||
        (ptt_spi_rx_words[2] != CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS) ||
        (ptt_spi_rx_words[3] != CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_STATUS)) {
        return CODEC_SERVICE_ERR_DECODE;
    }

    return CODEC_SERVICE_OK;
}

/**
 * @brief 将当前已编码整段 AMR 数据按 PTT SPI block 上传到 Art-Pi。
 *
 * 该接口只负责 DSP 到本地 Art-Pi 的整段 AMR 搬运：每个上传 block 后
 * 追加一次 DSP_STATUS_POLL，用于读取 Art-Pi 对上一 block 的延迟 ACK。
 * 本阶段不做 UDP、不做重传，也不修改 AMR 编码结果。
 *
 * @return int16_t CODEC_SERVICE_OK 表示上传完成，负值表示握手、长度或远端状态错误。
 */
int16_t codec_service_ptt_spi_upload_encoded(void)
{
    const uint8_t *amr_payload;
    Uint16 session_id;
    Uint16 block_count;
    Uint16 block_id;
    Uint16 payload_bytes;
    Uint16 offset;
    int16_t result;

    if (amr_len == 0U) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    if (amr_len > (Uint16)CODEC_SERVICE_AMR_BUF_SIZE - 1U) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    ptt_spi_session_id++;
    // 溢出保护：session_id 0 被保留给 DSP 主动发起的状态探针，上传 session 从 1 开始
    if (ptt_spi_session_id == 0U) {
        ptt_spi_session_id = 1U;
    }
    session_id = ptt_spi_session_id;
    amr_payload = &amr_output_buffer[1];
    // 以最大承载量分块
    block_count = (Uint16)((amr_len + CODEC_SERVICE_PTT_SPI_MAX_PAYLOAD_BYTES - 1U) /
                           CODEC_SERVICE_PTT_SPI_MAX_PAYLOAD_BYTES);
    offset = 0U;

    for (block_id = 0U; block_id < block_count; block_id++) {
        payload_bytes = (Uint16)(amr_len - offset);
        if (payload_bytes > CODEC_SERVICE_PTT_SPI_MAX_PAYLOAD_BYTES) {
            payload_bytes = CODEC_SERVICE_PTT_SPI_MAX_PAYLOAD_BYTES;
        }

        codec_service_ptt_spi_clear_words();
        codec_service_ptt_spi_fill_header(CODEC_SERVICE_PTT_SPI_BLOCK_DSP_UPLOAD_BLOCK,
                                          session_id,
                                          block_id,
                                          block_count,
                                          payload_bytes);
        // 将当前 block 的 AMR byte payload 打包到 SPI tx 16bit 缓冲区
        // 其实以前把8bit的数据当成16bit传也是可以的，但这样更清晰地表达了 byte 到 word 的关系，
        // 也方便后续扩展其他类型的 block
        codec_service_ptt_spi_pack_payload(&amr_payload[offset], payload_bytes);

        // 执行一次SPI传输，把当前 block 的 header 和 payload 从 DSP 传到 Art-Pi
        result = codec_service_ptt_spi_transfer_block();
        if (result != CODEC_SERVICE_OK) {
            return result;
        }
        
        // 这段代码不是在发语音数据，而是在向 Art-Pi 要“上一块语音数据收没收到、存没存好”的确认
        codec_service_ptt_spi_clear_words();
        codec_service_ptt_spi_fill_header(CODEC_SERVICE_PTT_SPI_BLOCK_DSP_STATUS_POLL,
                                          session_id,
                                          block_id,
                                          block_count,
                                          0U);

        result = codec_service_ptt_spi_transfer_block();
        if (result != CODEC_SERVICE_OK) {
            return result;
        }

        // 检查 Art-Pi 返回的 ACK 状态，确认上一块数据已经正确收到并存储；
        // 如果 ACK 不对，直接返回错误，不继续发后续块了
        // 后续版本可以在这里加重传机制，但目前第一版先不做重传了，直接暴露错误让上层处理
        result = codec_service_ptt_spi_check_status(session_id, block_id);
        if (result != CODEC_SERVICE_OK) {
            return result;
        }

        offset = (Uint16)(offset + payload_bytes);
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
                                   ptt_pcm_decode_frame,
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
            codec_workbuf.play_buf[play_sample_count++] = ptt_pcm_decode_frame[i];
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

/**
 * @brief 获取最近一次 PTT SPI block 探针收到的 word 缓冲区。
 *
 * 主状态机只用它打印调试头字段，不应长期保存该指针；下一次 SPI 收包会覆盖
 * 同一块接收缓冲。
 *
 * @return const Uint16* 指向 SPI 接收缓冲的 16-bit word 视图。
 */
const Uint16 *codec_service_get_ptt_spi_rx_words(void)
{
    return ptt_spi_rx_words;
}

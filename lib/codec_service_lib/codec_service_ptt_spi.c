#include "codec_service_internal.h"

#include "spi.h"
#include <string.h>

#pragma DATA_SECTION(ptt_spi_tx_words, "ZONE7DATA");
static Uint16 ptt_spi_tx_words[CODEC_SERVICE_SPI_PACKET_SIZE];

#pragma DATA_SECTION(ptt_spi_rx_words, "ZONE7DATA");
static Uint16 ptt_spi_rx_words[CODEC_SERVICE_SPI_PACKET_SIZE];

static Uint16 ptt_spi_session_id = 0;

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
// ART-PI填充到回复DSP的SPI block header，表示上一次处理结果，DSP通过轮询查询
#define CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_STATUS          5U
// DSP填充到SPI block header的状态查询，Art-Pi在收到该block时把上一次处理结果返回给DSP
#define CODEC_SERVICE_PTT_SPI_BLOCK_DSP_STATUS_POLL       6U

#define CODEC_SERVICE_PTT_SPI_STATUS_OK                   0U
#define CODEC_SERVICE_PTT_SPI_PAYLOAD_WORD0               CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS
#define CODEC_SERVICE_PTT_SPI_MAX_PAYLOAD_BYTES \
    ((CODEC_SERVICE_SPI_PACKET_SIZE - CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS) * 2U)

/**
 * @brief 复位 PTT SPI block 上传模块内部状态。
 *
 * 该函数由 codec_service_reset() 调用，只清空 session 计数，不改动 SPI
 * 底层或 GPIO 状态。
 *
 * @return void
 */
void codec_service_ptt_spi_reset_state(void)
{
    ptt_spi_session_id = 0;
}

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
 * @brief 将 SPI 16-bit word payload 解包为 AMR byte 数据。
 *
 * Art-Pi 下行下载时使用与 DSP 上传相同的 byte/word 规则：每个 word 的
 * 高字节是前一个 AMR byte，低字节是后一个 AMR byte；payload 为奇数字节
 * 时最后一个 word 的低字节是补零，不写入目标缓存。
 *
 * @param dst 输出 AMR byte 缓冲区。
 * @param payload_bytes 需要解包的逻辑 payload byte 数。
 *
 * @return int16_t CODEC_SERVICE_OK 表示解包成功，负值表示长度非法。
 */
static int16_t codec_service_ptt_spi_unpack_payload(uint8_t *dst, Uint16 payload_bytes)
{
    Uint16 i;
    Uint16 word_count;
    Uint16 word;

    if ((dst == 0) || (payload_bytes == 0U)) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    word_count = (Uint16)((payload_bytes + 1U) / 2U);
    if ((CODEC_SERVICE_PTT_SPI_PAYLOAD_WORD0 + word_count) > CODEC_SERVICE_SPI_PACKET_SIZE) {
        return CODEC_SERVICE_ERR_LENGTH;
    }

    for (i = 0U; i < word_count; i++) {
        word = ptt_spi_rx_words[CODEC_SERVICE_PTT_SPI_PAYLOAD_WORD0 + i];
        dst[i * 2U] = (uint8_t)((word >> 8) & 0x00FFU);
        if (((i * 2U) + 1U) < payload_bytes) {
            dst[(i * 2U) + 1U] = (uint8_t)(word & 0x00FFU);
        }
    }

    return CODEC_SERVICE_OK;
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
 * @brief 从 Art-Pi 下载已经收齐的远端 AMR session。
 *
 * 该接口只在 DATA_READY 为高时使用。DSP 每次主动发送 DSP_DOWNLOAD_REQ，
 * Art-Pi 在同一次 SPI transaction 中返回预装好的 ARTPI_DOWNLOAD_BLOCK。
 * DSP 按 block_id 顺序解包到 spi_receive_buffer + 1，并设置 received_amr_len，
 * 后续复用 codec_service_decode_received() 解码播放。
 *
 * @return int16_t CODEC_SERVICE_OK 表示下载完成，负值表示无数据、握手失败或 block 校验失败。
 */
int16_t codec_service_ptt_spi_download_received(void)
{
    Uint16 block_id;
    Uint16 block_count;
    Uint16 rx_type;
    Uint16 rx_session_id;
    Uint16 rx_block_id;
    Uint16 rx_block_count;
    Uint16 payload_bytes;
    Uint16 received_bytes;
    Uint16 session_id;
    int16_t result;

    if (spi_ptt_is_data_ready() == 0U) {
        return CODEC_SERVICE_ERR_NO_RECORD;
    }

    received_amr_len = 0U;
    received_bytes = 0U;
    session_id = 0U;
    block_count = 0U;

    for (block_id = 0U; ; block_id++) {
        codec_service_ptt_spi_clear_words();
        codec_service_ptt_spi_fill_header(CODEC_SERVICE_PTT_SPI_BLOCK_DSP_DOWNLOAD_REQ,
                                          session_id,
                                          block_id,
                                          block_count,
                                          0U);

        result = codec_service_ptt_spi_transfer_block();
        if (result != CODEC_SERVICE_OK) {
            received_amr_len = 0U;
            return result;
        }

        if ((ptt_spi_rx_words[0] != CODEC_SERVICE_PTT_SPI_BLOCK_MAGIC) ||
            (ptt_spi_rx_words[1] != CODEC_SERVICE_PTT_SPI_BLOCK_VERSION) ||
            (ptt_spi_rx_words[2] != CODEC_SERVICE_PTT_SPI_BLOCK_HEADER_WORDS)) {
            received_amr_len = 0U;
            return CODEC_SERVICE_ERR_DECODE;
        }

        rx_type = ptt_spi_rx_words[3];
        if (rx_type == CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_NO_SESSION) {
            received_amr_len = 0U;
            return (block_id == 0U) ? CODEC_SERVICE_ERR_NO_RECORD : CODEC_SERVICE_ERR_REMOTE;
        }

        if (rx_type != CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_DOWNLOAD_BLOCK) {
            received_amr_len = 0U;
            return CODEC_SERVICE_ERR_DECODE;
        }

        rx_session_id = ptt_spi_rx_words[4];
        rx_block_id = ptt_spi_rx_words[5];
        rx_block_count = ptt_spi_rx_words[6];
        payload_bytes = ptt_spi_rx_words[7];

        if (block_id == 0U) {
            if (rx_block_count == 0U) {
                received_amr_len = 0U;
                return CODEC_SERVICE_ERR_LENGTH;
            }
            session_id = rx_session_id;
            block_count = rx_block_count;
        }

        if ((rx_session_id != session_id) ||
            (rx_block_id != block_id) ||
            (rx_block_count != block_count) ||
            (payload_bytes == 0U)) {
            received_amr_len = 0U;
            return CODEC_SERVICE_ERR_REMOTE;
        }

        if (((Uint32)received_bytes + payload_bytes) > (CODEC_SERVICE_AMR_BUF_SIZE - 1U)) {
            received_amr_len = 0U;
            return CODEC_SERVICE_ERR_LENGTH;
        }

        result = codec_service_ptt_spi_unpack_payload(&spi_receive_buffer[1U + received_bytes],
                                                      payload_bytes);
        if (result != CODEC_SERVICE_OK) {
            received_amr_len = 0U;
            return result;
        }

        received_bytes = (Uint16)(received_bytes + payload_bytes);
        if ((block_id + 1U) >= block_count) {
            break;
        }
    }

    received_amr_len = received_bytes;
    return CODEC_SERVICE_OK;
}

/**
 * @brief 执行一次 PTT SPI block 状态探针。
 *
 * 该函数用于验证 DSP_REQ -> SPI_READY -> SPI clock 的三线握手顺序。
 * DSP 先拉高 DSP_REQ，请求 Art-Pi arm SPI slave DMA；等 SPI_READY 为高后，
 * 发送一个最小 DSP_DOWNLOAD_REQ block，并期望收到 Art-Pi 的 ARTPI_STATUS
 * block。
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
    if (ptt_spi_session_id == 0U) {
        ptt_spi_session_id = 1U;
    }
    session_id = ptt_spi_session_id;
    amr_payload = &amr_output_buffer[1];
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
        codec_service_ptt_spi_pack_payload(&amr_payload[offset], payload_bytes);

        result = codec_service_ptt_spi_transfer_block();
        if (result != CODEC_SERVICE_OK) {
            return result;
        }

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

        result = codec_service_ptt_spi_check_status(session_id, block_id);
        if (result != CODEC_SERVICE_OK) {
            return result;
        }

        offset = (Uint16)(offset + payload_bytes);
    }

    return CODEC_SERVICE_OK;
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

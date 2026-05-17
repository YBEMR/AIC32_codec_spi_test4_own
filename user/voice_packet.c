#include "voice_packet.h"

/**
 * @brief 校验语音包类型是否合法。
 *
 * 该函数用于判断传入的 packet_type 是否属于当前协议支持的语音包类型。
 * 支持的类型包括查询包、语音帧包、结束包、无数据包和状态包。
 *
 * @param packet_type 待校验的语音包类型。
 *
 * @return int16_t
 * @retval VOICE_PACKET_OK 类型合法。
 * @retval VOICE_PACKET_ERR_TYPE 类型非法。
 */
static int16_t voice_packet_validate_type(uint16_t packet_type)
{
    if ((packet_type == VOICE_PACKET_TYPE_QUERY) ||
        (packet_type == VOICE_PACKET_TYPE_FRAME) ||
        (packet_type == VOICE_PACKET_TYPE_END) ||
        (packet_type == VOICE_PACKET_TYPE_NO_DATA) ||
        (packet_type == VOICE_PACKET_TYPE_STATUS)) {
        return VOICE_PACKET_OK;
    }

    return VOICE_PACKET_ERR_TYPE;
}

/**
 * @brief 根据 payload 字节数计算其占用的 16-bit word 数量。
 *
 * 协议中 payload 以 16-bit word 形式传输，每两个 byte 打包成一个 word。
 * 如果 payload 字节数为奇数，则最后一个 word 的低 8 位补 0。
 *
 * @param payload_bytes payload 的有效字节数。
 *
 * @return uint16_t payload 占用的 16-bit word 数量。
 */
uint16_t voice_packet_payload_words(uint16_t payload_bytes)
{
    return (uint16_t)((payload_bytes + 1U) / 2U);  // 向上取整，确保奇数字节也占用一个 word
}

/**
 * @brief 根据 payload 字节数计算完整语音包占用的 16-bit word 数量。
 *
 * 总 word 数 = 固定协议头 word 数 + payload 占用的 word 数。
 *
 * @param payload_bytes payload 的有效字节数。
 *
 * @return uint16_t 完整语音包占用的 16-bit word 数量。
 */
uint16_t voice_packet_total_words(uint16_t payload_bytes)
{
    return (uint16_t)(VOICE_PACKET_HEADER_WORDS + voice_packet_payload_words(payload_bytes));
}

/**
 * @brief 将语音包元信息和 payload 编码成协议规定的 16-bit word 数据包。
 *
 * 该函数会先写入固定协议头，再将 payload 按两个 byte 合成一个
 * 16-bit word 的方式写入目标缓冲区。第一个 byte 放在高 8 位，
 * 第二个 byte 放在低 8 位。如果 payload 字节数为奇数，最后一个
 * word 的低 8 位补 0。即在协议中使用 big-endian 方式存储 payload 字节，和 AMR 字节顺序一致。
 *
 * @param dst_words_buf 输出缓冲区，用于存放编码后的 16-bit word 数据包。
 * @param dst_word_cap 输出缓冲区可容纳的 16-bit word 数量。
 * @param meta 输入参数，指向语音包元信息结构体。
 * @param payload_bytes_buf 输入参数，指向 payload 字节数据；当 payload_bytes_buf 为 0 时可为空。
 * @param out_total_words 输出参数，用于返回编码后数据包的有效 word 总数。
 *
 * @return int16_t
 * @retval VOICE_PACKET_OK 编码成功。
 * @retval VOICE_PACKET_ERR_NULL 输入指针为空。
 * @retval VOICE_PACKET_ERR_TYPE 语音包类型非法。
 * @retval VOICE_PACKET_ERR_PAYLOAD payload 长度超过协议限制。
 * @retval VOICE_PACKET_ERR_LENGTH 输出缓冲区容量不足。
 */
int16_t voice_packet_encode(uint16_t *dst_words_buf,
                            uint16_t dst_word_cap,
                            const voice_packet_meta_t *meta,
                            const uint8_t *payload_bytes_buf,
                            uint16_t *out_total_words)
{
    uint16_t payload_words;
    uint16_t total_words;
    uint16_t i;

    if ((dst_words_buf == 0) || (meta == 0) || (out_total_words == 0)) {
        return VOICE_PACKET_ERR_NULL;
    }

    if ((meta->payload_bytes != 0U) && (payload_bytes_buf == 0)) {
        return VOICE_PACKET_ERR_NULL;
    }

    if (voice_packet_validate_type(meta->packet_type) != VOICE_PACKET_OK) {
        return VOICE_PACKET_ERR_TYPE;
    }

    if (meta->payload_bytes > VOICE_PACKET_MAX_PAYLOAD_BYTES) {
        return VOICE_PACKET_ERR_PAYLOAD;
    }

    payload_words = voice_packet_payload_words(meta->payload_bytes);
    total_words = (uint16_t)(VOICE_PACKET_HEADER_WORDS + payload_words);

    if (dst_word_cap < total_words) {
        return VOICE_PACKET_ERR_LENGTH;
    }

    /*
     * 前 12 个 word 是固定协议头。
     * 后续接入 SPI/UDP 时，只需要发送 total_words 个有效 word，
     * 不再把整个 8192-word 缓冲区都发出去。
     */
    dst_words_buf[VOICE_PACKET_OFF_MAGIC] = VOICE_PACKET_MAGIC;
    dst_words_buf[VOICE_PACKET_OFF_VERSION] = VOICE_PACKET_VERSION;
    dst_words_buf[VOICE_PACKET_OFF_HEADER_WORDS] = VOICE_PACKET_HEADER_WORDS;
    dst_words_buf[VOICE_PACKET_OFF_TYPE] = meta->packet_type;
    dst_words_buf[VOICE_PACKET_OFF_FLAGS] = meta->flags;
    dst_words_buf[VOICE_PACKET_OFF_SESSION_ID] = meta->session_id;
    dst_words_buf[VOICE_PACKET_OFF_FRAME_ID] = meta->frame_id;
    dst_words_buf[VOICE_PACKET_OFF_FRAME_COUNT] = meta->frame_count;
    dst_words_buf[VOICE_PACKET_OFF_PAYLOAD_BYTES] = meta->payload_bytes;
    dst_words_buf[VOICE_PACKET_OFF_PAYLOAD_WORDS] = payload_words;
    dst_words_buf[VOICE_PACKET_OFF_RESERVED0] = 0U;
    dst_words_buf[VOICE_PACKET_OFF_RESERVED1] = 0U;

    /*
     * AMR payload 本身是按 byte 表示的，这里把两个 byte 合成一个
     * 16-bit word：第一个 byte 放高 8 位，第二个 byte 放低 8 位。
     * 如果 payload 是奇数字节，最后一个 word 的低 8 位补 0；
     * 解包时只按照 payload_bytes 取有效字节，补零不会被交给 AMR 解码。
     * 0x0023 0x0021 -> 0x2321
     */
    for (i = 0U; i < payload_words; i++) {
        uint16_t byte_index = (uint16_t)(i * 2U);
        uint16_t word = (uint16_t)((payload_bytes_buf[byte_index] & 0x00FFU) << 8);

        if ((uint16_t)(byte_index + 1U) < meta->payload_bytes) {
            word |= (uint16_t)(payload_bytes_buf[byte_index + 1U] & 0x00FFU);
        }

        dst_words_buf[VOICE_PACKET_HEADER_WORDS + i] = word;
    }

    *out_total_words = total_words;
    return VOICE_PACKET_OK;
}

/**
 * @brief 解析并校验语音包协议头。
 *
 * 该函数会检查输入数据包的长度、魔数、协议版本、协议头长度、
 * 数据包类型以及 payload 长度字段是否合法。解析成功后，会将协议头中的
 * 元信息写入 out_meta。
 *
 * 注意：该函数只解析协议头和检查长度，不会提取 payload 内容。
 *
 * @param src_words_buf 输入参数，指向接收到的 16-bit word 数据包。
 * @param src_word_len 输入数据包中可用的 16-bit word 数量。
 * @param out_meta 输出参数，用于保存解析得到的语音包元信息。
 *
 * @return int16_t
 * @retval VOICE_PACKET_OK 解析成功。
 * @retval VOICE_PACKET_ERR_NULL 输入指针为空。
 * @retval VOICE_PACKET_ERR_LENGTH 数据包长度不合法或长度字段不匹配。
 * @retval VOICE_PACKET_ERR_MAGIC 魔数不匹配。
 * @retval VOICE_PACKET_ERR_VERSION 协议版本不匹配。
 * @retval VOICE_PACKET_ERR_TYPE 语音包类型非法。
 * @retval VOICE_PACKET_ERR_PAYLOAD payload 长度超过协议限制。
 */
int16_t voice_packet_decode_header(const uint16_t *src_words_buf,
                                   uint16_t src_word_len,
                                   voice_packet_meta_t *out_meta)
{
    uint16_t payload_bytes;
    uint16_t payload_words;
    uint16_t expected_payload_words;
    uint16_t total_words;

    if ((src_words_buf == 0) || (out_meta == 0)) {
        return VOICE_PACKET_ERR_NULL;
    }

    if (src_word_len < VOICE_PACKET_HEADER_WORDS) {
        return VOICE_PACKET_ERR_LENGTH;
    }

    if (src_words_buf[VOICE_PACKET_OFF_MAGIC] != VOICE_PACKET_MAGIC) {
        return VOICE_PACKET_ERR_MAGIC;
    }

    if (src_words_buf[VOICE_PACKET_OFF_VERSION] != VOICE_PACKET_VERSION) {
        return VOICE_PACKET_ERR_VERSION;
    }

    if (src_words_buf[VOICE_PACKET_OFF_HEADER_WORDS] != VOICE_PACKET_HEADER_WORDS) {
        return VOICE_PACKET_ERR_LENGTH;
    }

    if (voice_packet_validate_type(src_words_buf[VOICE_PACKET_OFF_TYPE]) != VOICE_PACKET_OK) {
        return VOICE_PACKET_ERR_TYPE;
    }

    payload_bytes = src_words_buf[VOICE_PACKET_OFF_PAYLOAD_BYTES];
    payload_words = src_words_buf[VOICE_PACKET_OFF_PAYLOAD_WORDS];

    if (payload_bytes > VOICE_PACKET_MAX_PAYLOAD_BYTES) {
        return VOICE_PACKET_ERR_PAYLOAD;
    }

    /*
     * payload_words 必须由 payload_bytes 推导得到。
     * 这个检查可以在读取 payload 区域之前发现长度字段被破坏、
     * 收包被截断或两端协议版本不一致等问题。
     */
    expected_payload_words = voice_packet_payload_words(payload_bytes);
    if (payload_words != expected_payload_words) {
        return VOICE_PACKET_ERR_LENGTH;
    }

    total_words = (uint16_t)(VOICE_PACKET_HEADER_WORDS + payload_words);
    if (src_word_len < total_words) {
        return VOICE_PACKET_ERR_LENGTH;
    }

    out_meta->packet_type = src_words_buf[VOICE_PACKET_OFF_TYPE];
    out_meta->flags = src_words_buf[VOICE_PACKET_OFF_FLAGS];
    out_meta->session_id = src_words_buf[VOICE_PACKET_OFF_SESSION_ID];
    out_meta->frame_id = src_words_buf[VOICE_PACKET_OFF_FRAME_ID];
    out_meta->frame_count = src_words_buf[VOICE_PACKET_OFF_FRAME_COUNT];
    out_meta->payload_bytes = payload_bytes;
    out_meta->payload_words = payload_words;
    out_meta->total_words = total_words;

    return VOICE_PACKET_OK;
}

/**
 * @brief 从语音包中提取 payload 字节数据。
 *
 * 该函数会先调用 voice_packet_decode_header() 对协议头进行解析和校验，
 * 然后根据协议头中的 payload_bytes 字段，从 16-bit word 数据区中恢复
 * 原始 byte 形式的 payload。
 *
 * payload 编码格式为：每个 16-bit word 保存两个 byte，
 * 高 8 位为偶数下标 byte，低 8 位为奇数下标 byte。
 *
 * @param src_words 输入参数，指向接收到的 16-bit word 数据包。
 * @param src_word_len 输入数据包中可用的 16-bit word 数量。
 * @param dst_payload 输出缓冲区，用于保存提取出的 payload 字节数据。
 * @param dst_payload_cap 输出缓冲区可容纳的字节数。
 * @param out_payload_bytes 输出参数，用于返回实际提取出的 payload 字节数。
 *
 * @return int16_t
 * @retval VOICE_PACKET_OK payload 提取成功。
 * @retval VOICE_PACKET_ERR_NULL 输入或输出指针为空。
 * @retval VOICE_PACKET_ERR_LENGTH 数据包长度不合法或长度字段不匹配。
 * @retval VOICE_PACKET_ERR_MAGIC 魔数不匹配。
 * @retval VOICE_PACKET_ERR_VERSION 协议版本不匹配。
 * @retval VOICE_PACKET_ERR_TYPE 语音包类型非法。
 * @retval VOICE_PACKET_ERR_PAYLOAD payload 长度非法或输出缓冲区容量不足。
 */
int16_t voice_packet_extract_payload(const uint16_t *src_words_buf,
                                     uint16_t src_word_len,
                                     uint8_t *dst_payload,
                                     uint16_t dst_payload_cap,
                                     uint16_t *out_payload_bytes)
{
    voice_packet_meta_t meta;
    int16_t result;
    uint16_t i;

    if (out_payload_bytes == 0) {
        return VOICE_PACKET_ERR_NULL;
    }

    result = voice_packet_decode_header(src_words_buf, src_word_len, &meta);
    if (result != VOICE_PACKET_OK) {
        return result;
    }

    if ((meta.payload_bytes != 0U) && (dst_payload == 0)) {
        return VOICE_PACKET_ERR_NULL;
    }

    if (dst_payload_cap < meta.payload_bytes) {
        return VOICE_PACKET_ERR_PAYLOAD;
    }

    for (i = 0U; i < meta.payload_bytes; i++) {
        uint16_t word = src_words_buf[VOICE_PACKET_HEADER_WORDS + (i / 2U)];
        // 偶数下标 byte 在 word 的高 8 位，奇数下标 byte 在 word 的低 8 位
        if ((i & 1U) == 0U) {
            dst_payload[i] = (uint8_t)((word >> 8) & 0x00FFU);
        } else {
            dst_payload[i] = (uint8_t)(word & 0x00FFU);
        }
    }

    *out_payload_bytes = meta.payload_bytes;
    return VOICE_PACKET_OK;
}

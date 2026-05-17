#ifndef VOICE_PACKET_H_
#define VOICE_PACKET_H_

#include <stdint.h>

/*
 * PTT 语音包协议统一使用 16-bit word 作为传输基本单位。
 *
 * DSP 和 Art-Pi 两端必须按照下面的 word 偏移读写字段，不能改成
 * packed struct 或直接传结构体。这样可以避开 C2000 DSP 与 ARM MCU
 * 在结构体对齐、字节序表达上的差异，保证 SPI 与 UDP 看到的布局一致。
 */
#define VOICE_PACKET_MAGIC              0x5650U
#define VOICE_PACKET_VERSION            0x0001U
#define VOICE_PACKET_HEADER_WORDS       12U
#define VOICE_PACKET_MAX_PAYLOAD_BYTES  32U
#define VOICE_PACKET_MAX_PAYLOAD_WORDS  16U
#define VOICE_PACKET_MAX_WORDS          (VOICE_PACKET_HEADER_WORDS + VOICE_PACKET_MAX_PAYLOAD_WORDS)

#define VOICE_PACKET_OFF_MAGIC          0U
#define VOICE_PACKET_OFF_VERSION        1U
#define VOICE_PACKET_OFF_HEADER_WORDS   2U
#define VOICE_PACKET_OFF_TYPE           3U
#define VOICE_PACKET_OFF_FLAGS          4U
#define VOICE_PACKET_OFF_SESSION_ID     5U
#define VOICE_PACKET_OFF_FRAME_ID       6U
#define VOICE_PACKET_OFF_FRAME_COUNT    7U
#define VOICE_PACKET_OFF_PAYLOAD_BYTES  8U
#define VOICE_PACKET_OFF_PAYLOAD_WORDS  9U
#define VOICE_PACKET_OFF_RESERVED0      10U
#define VOICE_PACKET_OFF_RESERVED1      11U

#define VOICE_PACKET_TYPE_QUERY         1U
#define VOICE_PACKET_TYPE_FRAME         2U
#define VOICE_PACKET_TYPE_END           3U
#define VOICE_PACKET_TYPE_NO_DATA       4U
#define VOICE_PACKET_TYPE_STATUS        5U

#define VOICE_PACKET_FLAG_FIRST         0x0001U
#define VOICE_PACKET_FLAG_LAST          0x0002U

#define VOICE_PACKET_OK                 0
#define VOICE_PACKET_ERR_NULL          -1
#define VOICE_PACKET_ERR_MAGIC         -2
#define VOICE_PACKET_ERR_VERSION       -3
#define VOICE_PACKET_ERR_TYPE          -4
#define VOICE_PACKET_ERR_LENGTH        -5
#define VOICE_PACKET_ERR_PAYLOAD       -6

typedef struct {
    uint16_t packet_type;
    uint16_t flags;
    uint16_t session_id;
    uint16_t frame_id;
    uint16_t frame_count;
    uint16_t payload_bytes;
    uint16_t payload_words;
    uint16_t total_words;
} voice_packet_meta_t;

uint16_t voice_packet_payload_words(uint16_t payload_bytes);
uint16_t voice_packet_total_words(uint16_t payload_bytes);

int16_t voice_packet_encode(uint16_t *dst_words,
                            uint16_t dst_word_cap,
                            const voice_packet_meta_t *meta,
                            const uint8_t *payload_bytes,
                            uint16_t *out_total_words);

int16_t voice_packet_decode_header(const uint16_t *src_words,
                                   uint16_t src_word_len,
                                   voice_packet_meta_t *out_meta);

int16_t voice_packet_extract_payload(const uint16_t *src_words,
                                     uint16_t src_word_len,
                                     uint8_t *dst_payload,
                                     uint16_t dst_payload_cap,
                                     uint16_t *out_payload_bytes);

#endif /* VOICE_PACKET_H_ */

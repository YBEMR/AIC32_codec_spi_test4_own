#ifndef CODEC_SERVICE_INTERNAL_H_
#define CODEC_SERVICE_INTERNAL_H_

#include "codec_service.h"

typedef union {
    int16_t record_buf[CODEC_SERVICE_MAX_RECORD_CNT];
    int16_t play_buf[CODEC_SERVICE_MAX_RECORD_CNT];
} codec_service_workbuf_t;

extern codec_service_workbuf_t codec_workbuf;
extern uint8_t amr_output_buffer[CODEC_SERVICE_AMR_BUF_SIZE];
extern uint8_t spi_receive_buffer[CODEC_SERVICE_AMR_BUF_SIZE];
extern Uint32 record_count;
extern Uint16 amr_len;
extern Uint16 received_amr_len;
extern Uint32 play_sample_count;
extern Uint32 play_sample_offset;

void codec_service_ptt_spi_reset_state(void);
void codec_service_ptt_frame_reset_state(void);

#endif /* CODEC_SERVICE_INTERNAL_H_ */

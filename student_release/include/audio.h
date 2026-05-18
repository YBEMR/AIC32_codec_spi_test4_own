#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "DSP2833x_Device.h"   
#include "DSP2833x_Examples.h" 
#include "tw_amr.h"
#include "wavreader.h"

void AIC23Init(void);
void I2CA_Init(void);

typedef Uint32 (*audio_tick_getter_t)(void);
void audio_set_tick_getter(audio_tick_getter_t getter);

#define AMR_PCM_FRAME_SAMPLES 160U
#define AMR_FRAME_MAX_BYTES   AMR_IETF_MAX_PL
/*
 * header: buffer of PCM data to which WAV header will be added
 * data_size: size of PCM data in bytes
 */
void create_wav_header(uint8_t *header, uint32_t data_size);

/*
 * src: buffer of 16-bit data
 * dst: buffer for converted 8-bit data
 * num_samples: number of 16-bit samples
 */
void convert_16bit_to_8bit(const int16_t *src, uint8_t *dst, uint32_t num_samples);

/*
 * wav_data: WAV data
 * wav_len: length of 8-bit WAV data in bytes
 * amr_buf: buffer for encoded AMR data
 * amr_buf_size: maximum size of encoded AMR data buffer
 * amr_len: actual length of encoded AMR data
 */
int16_t amr_encode_wav(const uint8_t *wav_data, uint32_t wav_len,
                  uint8_t *amr_buf, uint16_t amr_buf_size,
                  uint16_t *amr_len);

/*
 * pcm_data: PCM16 sample data
 * sample_count: number of PCM16 samples
 * amr_buf: buffer for encoded AMR data
 * amr_buf_size: maximum size of encoded AMR data buffer
 * amr_len: actual length of encoded AMR data
 */
int16_t amr_encode_pcm16(const int16_t *pcm_data, uint32_t sample_count,
                    uint8_t *amr_buf, uint16_t amr_buf_size,
                    uint16_t *amr_len);

int16_t amr_encode_frame_reset(void);
int16_t amr_encode_pcm16_frame(const int16_t *pcm_frame,
                    uint8_t *amr_frame, uint16_t amr_frame_buf_size,
                    uint16_t *amr_frame_len);

/*
 * amr_data: AMR data
 * amr_len: length of AMR data in bytes
 * wav_ptr: buffer for decoded WAV data
 * wav_buf_size: maximum size of decoded WAV data buffer
 * wav_len: actual length of decoded WAV data
 */
int16_t amr_decode_wav(const uint8_t *amr_data, uint16_t amr_len,
                  uint8_t *wav_ptr, uint32_t wav_buf_size,
                  uint32_t *wav_len);

int16_t amr_decode_frame_reset(void);
int16_t amr_ietf_frame_length(uint8_t first_octet, uint16_t *frame_len);
int16_t amr_decode_pcm16_frame(const uint8_t *amr_frame,
                    uint16_t amr_frame_len,
                    int16_t *pcm_frame,
                    uint16_t pcm_sample_capacity,
                    uint16_t *pcm_sample_count);

void convert_8bit_to_16bit(const uint8_t *src, uint16_t *dst, uint32_t num_bytes);
void __amr_decoder_create(struct amr_decoder_state *st);
void __write_pcm_to_wav(uint8_t **out_buf, const int16_t *pcm, uint32_t *data_length);
void __write_string(uint8_t **data_buf, const char *str);
void __write_int32(uint8_t **data_buf, int32_t value);
void __write_int16(uint8_t **data_buf, int16_t value);
void __write_header(uint8_t *ptr_data_buf, uint32_t length);







#endif  //_AUDIO_H_

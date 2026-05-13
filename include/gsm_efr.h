/*
 * This header file is the external public interface to libgsmefr:
 * Themyscira Wireless implementation of GSM-EFR speech codec
 * based on the original ETSI code of GSM 06.53, adapted to the
 * RTP frame format of ETSI TS 101 318.
 *
 * This header file should be installed in some system include directory
 * such that it can be included by C sources as <gsm_efr.h>.
 */

#ifndef	__GSM_EFR_H
#define	__GSM_EFR_H

#include <stdint.h>

#define	EFR_RTP_FRAME_LEN	31
#define	EFR_NUM_PARAMS		57

struct EFR_encoder_state;	/* opaque to external users */
struct EFR_decoder_state;	/* ditto */

extern struct EFR_encoder_state *EFR_encoder_create(int dtx);
extern struct EFR_decoder_state *EFR_decoder_create(void);
/* use standard free() call to free both afterward */

/* reset state to initial */
extern void EFR_encoder_reset(struct EFR_encoder_state *st, int dtx);
extern void EFR_decoder_reset(struct EFR_decoder_state *st);

/* encoder public functions */

extern void EFR_encode_params(struct EFR_encoder_state *st, const int16_t *pcm,
			      int16_t *params, int *sp, int *vad);
extern void EFR_encode_frame(struct EFR_encoder_state *st, const int16_t *pcm,
			     uint8_t *frame, int *sp, int *vad);

/* decoder public functions */

extern void EFR_decode_params(struct EFR_decoder_state *st,
			      const int16_t *params, int bfi, int sid, int taf,
			      int16_t *pcm);
extern void EFR_decode_frame(struct EFR_decoder_state *st, const uint8_t *frame,
			     int bfi, int taf, int16_t *pcm);
extern void EFR_decode_bfi_nodata(struct EFR_decoder_state *st, int taf,
				  int16_t *pcm);
extern int EFR_decode_rtp(struct EFR_decoder_state *st, const uint8_t *rtp_pl,
			  unsigned rtp_pl_len, int16_t *pcm);

/* stateless utility functions */

extern int EFR_sid_classify(const uint8_t *frame);
extern void EFR_frame2params(const uint8_t *frame, int16_t *params);
extern void EFR_params2frame(const int16_t *params, uint8_t *frame);
extern void EFR_insert_sid_codeword(uint8_t *frame);

/* public const data item */

extern const uint8_t EFR_decoder_homing_frame[EFR_RTP_FRAME_LEN];

/* sizes of state structures, to support alternative malloc schemes */

extern const unsigned EFR_encoder_state_size;
extern const unsigned EFR_decoder_state_size;

#endif	/* include guard */

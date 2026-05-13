/*
 * This header file is the external public interface to libgsmfr2:
 * Themyscira Wireless implementation of GSM FRv1 speech codec
 * that includes not only the core transcoding functions of GSM 06.10,
 * but also Rx DTX functions (substitution and muting per GSM 06.11,
 * comfort noise per GSM 06.12, overall Rx DTX control per GSM 06.31)
 * in the decoder direction and the optional homing frame mechanism.
 *
 * This header file should be installed in some system include directory
 * such that it can be included by C sources as <tw_gsmfr.h>.
 */

#ifndef	__THEMWI_GSMFR_H
#define	__THEMWI_GSMFR_H

#include <stdint.h>

#define	GSMFR_RTP_FRAME_LEN	33
#define	GSMFR_NUM_PARAMS	76

/*
 * GSM 06.10 encoder & decoder portion of the library
 *
 * This part is peculiar in that the same state structure is used for
 * both the encoder and the decoder - however, each given instance
 * of this state structure must be used for only one of the two.
 */

struct gsmfr_0610_state;	/* opaque to external users */

struct gsmfr_0610_state *gsmfr_0610_create(void);
/* use standard free() call to free it afterward */

/* reset state to initial */
void gsmfr_0610_reset(struct gsmfr_0610_state *state);

/* interface structure for passing frames of codec parameters */

struct gsmfr_param_frame {
	int16_t	LARc[8];
	int16_t	Nc[4];
	int16_t	bc[4];
	int16_t	Mc[4];
	int16_t	xmaxc[4];
	int16_t	xMc[4][13];
};

/* encoder public functions */

void gsmfr_0610_encode_params(struct gsmfr_0610_state *st, const int16_t *pcm,
			      struct gsmfr_param_frame *param);
void gsmfr_0610_encode_frame(struct gsmfr_0610_state *st, const int16_t *pcm,
			     uint8_t *frame);
void gsmfr_0610_encoder_homing(struct gsmfr_0610_state *st, const int16_t *pcm);

/* decoder public functions */

void gsmfr_0610_decode_params(struct gsmfr_0610_state *st,
			      const struct gsmfr_param_frame *param,
			      int16_t *pcm);
void gsmfr_0610_decode_frame(struct gsmfr_0610_state *st, const uint8_t *frame,
			     int16_t *pcm);

/* conversion between RTP packed format and struct gsmfr_param_frame */

void gsmfr_pack_frame(const struct gsmfr_param_frame *param, uint8_t *frame);
void gsmfr_unpack_frame(const uint8_t *frame, struct gsmfr_param_frame *param);

/* similar conversions with a linear array of params */

void gsmfr_pack_from_array(const int16_t *params, uint8_t *frame);
void gsmfr_unpack_to_array(const uint8_t *frame, int16_t *params);

/* Rx DTX handler preprocessor portion of the library */

struct gsmfr_preproc_state;	/* opaque to external users */

struct gsmfr_preproc_state *gsmfr_preproc_create(void);
/* use standard free() call to free it afterward */

/* reset state to initial */
void gsmfr_preproc_reset(struct gsmfr_preproc_state *state);

/* main processing functions */
void gsmfr_preproc_good_frame(struct gsmfr_preproc_state *state,
				uint8_t *frame);
void gsmfr_preproc_good_frame_hm(struct gsmfr_preproc_state *state,
				 uint8_t *frame);
void gsmfr_preproc_bfi(struct gsmfr_preproc_state *state, int taf,
			uint8_t *frame_out);
void gsmfr_preproc_bfi_bits(struct gsmfr_preproc_state *state,
			    const uint8_t *bad_frame, int taf,
			    uint8_t *frame_out);

/* utility function */
int gsmfr_preproc_sid_classify(const uint8_t *frame);

/* utility datum */
extern const uint8_t gsmfr_preproc_silence_frame[GSMFR_RTP_FRAME_LEN];

/*
 * Full GSM-FR decoder: Rx DTX handler followed by 06.10 decoder,
 * plus the decoder homing function.
 */

struct gsmfr_fulldec_state;	/* opaque to external users */

struct gsmfr_fulldec_state *gsmfr_fulldec_create(void);
/* use standard free() call to free it afterward */

/* reset state to initial */
void gsmfr_fulldec_reset(struct gsmfr_fulldec_state *state);

/* main processing functions */
void gsmfr_fulldec_good_frame(struct gsmfr_fulldec_state *state,
				const uint8_t *frame, int16_t *pcm);
void gsmfr_fulldec_bfi(struct gsmfr_fulldec_state *state, int taf,
			int16_t *pcm);
void gsmfr_fulldec_bfi_bits(struct gsmfr_fulldec_state *state,
			    const uint8_t *bad_frame, int taf, int16_t *pcm);

/* handling of TW-TS-001 RTP input */
int gsmfr_fulldec_rtp_in(struct gsmfr_fulldec_state *state,
			 const uint8_t *rtp_pl, unsigned rtp_pl_len,
			 int16_t *pcm);

/* utility datum */
extern const uint8_t gsmfr_decoder_homing_frame[GSMFR_RTP_FRAME_LEN];

/* TFO transform API based on the preprocessor part of the library */

int gsmfr_tfo_xfrm_main(struct gsmfr_preproc_state *state,
			const uint8_t *rtp_in, unsigned rtp_in_len,
			uint8_t *frame_out);
int gsmfr_tfo_xfrm_dtxd(struct gsmfr_preproc_state *state, uint8_t *frame_out);

/* sizes of state structures, to support alternative malloc schemes */

extern const unsigned gsmfr_0610_state_size;
extern const unsigned gsmfr_preproc_state_size;
extern const unsigned gsmfr_fulldec_state_size;

#endif	/* include guard */

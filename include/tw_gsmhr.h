/*
 * This header file is the external public interface to libgsmhr1:
 * Themyscira Wireless implementation of GSM HRv1 speech codec
 * based on the original ETSI code of GSM 06.06.
 *
 * This header file should be installed in some system include directory
 * such that it can be included by C sources as <tw_gsmhr.h>.
 */

#ifndef	__THEMWI_GSMHR_H
#define	__THEMWI_GSMHR_H

#include <stdint.h>

#define	GSMHR_NUM_PARAMS	18	/* actual codec parameters */
#define	GSMHR_NUM_PARAMS_ENC	20	/* output from the encoder */
#define	GSMHR_NUM_PARAMS_DEC	22	/* input to the decoder */

#define	GSMHR_FRAME_LEN_RPF	14	/* raw packed format */
#define	GSMHR_FRAME_LEN_5993	15	/* RFC 5993 and TW-TS-002 */

/* stateful encoder, decoder and TFO engines */

struct gsmhr_encoder_state;	/* opaque to external users */
struct gsmhr_decoder_state;	/* ditto */
struct gsmhr_rxfe_state;

struct gsmhr_encoder_state *gsmhr_encoder_create(int dtx);
struct gsmhr_decoder_state *gsmhr_decoder_create(void);
struct gsmhr_rxfe_state *gsmhr_rxfe_create(void);
/* use standard free() call to free these instances afterward */

/* reset state to initial */
void gsmhr_encoder_reset(struct gsmhr_encoder_state *st, int dtx);
void gsmhr_decoder_reset(struct gsmhr_decoder_state *st);
void gsmhr_rxfe_reset(struct gsmhr_rxfe_state *st);

/* encoder and decoder main functions */
void gsmhr_encode_frame(struct gsmhr_encoder_state *st, const int16_t *pcm,
			int16_t *param);
void gsmhr_decode_frame(struct gsmhr_decoder_state *st, const int16_t *param,
			int16_t *pcm);

/* TFO transform */
void gsmhr_tfo_xfrm(struct gsmhr_rxfe_state *st, int dtxd, const int16_t *ul,
		    int16_t *dl);

/* stateless format conversion functions */

void gsmhr_pack_ts101318(const int16_t *param, uint8_t *payload);
void gsmhr_unpack_ts101318(const uint8_t *payload, int16_t *param);

void gsmhr_encoder_twts002_out(const int16_t *param, uint8_t *payload);
int gsmhr_decoder_twts002_in(const uint8_t *payload, int16_t *param);

int gsmhr_rtp_in_preen(const uint8_t *rtp_in, unsigned rtp_in_len,
			uint8_t *canon_pl);
int gsmhr_rtp_in_direct(const uint8_t *rtp_in, unsigned rtp_in_len,
			int16_t *param);

/* reading parameter arrays from files: validation functions */

int gsmhr_check_common_params(const int16_t *params);
int gsmhr_check_encoder_params(const int16_t *params);
int gsmhr_check_decoder_params(const int16_t *params);

/* perfect SID detection and regeneration */

int gsmhr_ts101318_is_perfect_sid(const uint8_t *payload);
void gsmhr_ts101318_set_sid_codeword(uint8_t *payload);
void gsmhr_set_sid_cw_params(int16_t *params);

/* public const data items */

extern const int16_t gsmhr_dhf_params[GSMHR_NUM_PARAMS];
extern const uint8_t gsmhr_dhf_ts101318[GSMHR_FRAME_LEN_RPF];

#endif	/* include guard */

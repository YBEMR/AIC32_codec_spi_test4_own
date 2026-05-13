/*
 * This header file is the external public interface to libtwamr;
 * it should be installed in the same system include directory
 * as <tw_gsmfr.h> and <gsm_efr.h> for more classic GSM codecs.
 */

#ifndef	__THEMWI_AMR_H
#define	__THEMWI_AMR_H

#include <stdint.h>

/* AMR definitions that matter for the public interface */

#define	AMR_MAX_PRM		57	/* max. num. of params      */
#define	AMR_IETF_MAX_PL		32	/* max bytes in RFC 4867 frame */
#define	AMR_IETF_HDR_LEN	6	/* .amr file header bytes */
#define	AMR_COD_WORDS		250	/* # of words in 3GPP test seq format */



enum RXFrameType {
	RX_SPEECH_GOOD = 0,
	RX_SPEECH_DEGRADED,
	RX_ONSET,
	RX_SPEECH_BAD,
	RX_SID_FIRST,
	RX_SID_UPDATE,
	RX_SID_BAD,
	RX_NO_DATA,
	RX_N_FRAMETYPES		/* number of frame types */
};

enum TXFrameType {
	TX_SPEECH_GOOD = 0,
	TX_SID_FIRST,
	TX_SID_UPDATE,
	TX_NO_DATA,
	TX_SPEECH_DEGRADED,
	TX_SPEECH_BAD,
	TX_SID_BAD,
	TX_ONSET,
	TX_N_FRAMETYPES		/* number of frame types */
};

enum Mode {
	MR475 = 0,
	MR515,
	MR59,
	MR67,
	MR74,
	MR795,
	MR102,
	MR122,
	MRDTX
};

#define	AMR_FT_NODATA		15

/* libtwamr encoder and decoder state */


struct amr_encoder_state;	/* opaque to external users */
struct amr_decoder_state;	/* ditto */

struct amr_encoder_state *amr_encoder_create(int dtx, int use_vad2);
struct amr_decoder_state *amr_decoder_create(void);

/* reset state to initial */
void amr_encoder_reset(struct amr_encoder_state *st, int dtx, int use_vad2);
void amr_decoder_reset(struct amr_decoder_state *st);

/* interface structure for passing frames of codec parameters */

struct amr_param_frame {
	uint8_t	type;
	uint8_t	mode;
	int16_t	param[AMR_MAX_PRM];
};

/* encoder and decoder main functions */

void amr_encode_frame(struct amr_encoder_state *st, enum Mode mode,
			const int16_t *pcm, struct amr_param_frame *frame);

void amr_decode_frame(struct amr_decoder_state *st,
			const struct amr_param_frame *frame, int16_t *pcm);

/* stateless utility functions: format conversions */

int amr_txtype_to_rxtype(enum TXFrameType tx_type, enum RXFrameType *rx_type);

unsigned amr_frame_to_ietf(const struct amr_param_frame *frame, uint8_t *bytes);
int amr_frame_from_ietf(const uint8_t *bytes, struct amr_param_frame *frame);
int amr_ietf_grok_first_octet(uint8_t fo);

void amr_frame_to_tseq(const struct amr_param_frame *frame, uint16_t *cod);
int amr_frame_from_tseq(const uint16_t *cod, int use_rxtype,
			struct amr_param_frame *frame);

/* stateless function and const data for DHF detection */

int amr_check_dhf(const struct amr_param_frame *frame, int first_sub_only);

extern const int16_t amr_dhf_mr475[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr515[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr59[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr67[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr74[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr795[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr102[AMR_MAX_PRM];
extern const int16_t amr_dhf_mr122[AMR_MAX_PRM];
extern const int16_t amr_dhf_gsmefr[AMR_MAX_PRM];

/* DHF transformation on encoder output, for AMR-EFR emulation */

void amr_dhf_subst_efr(struct amr_param_frame *frame);
void amr_dhf_subst_efr2(struct amr_param_frame *frame, const int16_t *pcm);

/* public const datum: RFC 4867 file header */

extern const uint8_t amr_file_header_magic[AMR_IETF_HDR_LEN];

#endif	/* include guard */

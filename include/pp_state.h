/*
 * This header file is internal to libgsmfr2;
 * here we define our state structure for the Rx DTX preprocessor component.
 */

enum rx_dtx_st {
	NO_DATA = 0,
	SPEECH,
	SPEECH_MUTING,
	COMFORT_NOISE,
	LOST_SID,
	CN_MUTING,
};

struct gsmfr_preproc_state {
	enum rx_dtx_st	rx_state;
	uint8_t		speech_frame[GSMFR_RTP_FRAME_LEN];
	uint8_t		sid_prefix[5];
	uint8_t		sid_xmaxc;
	uint32_t	cn_random_lfsr;
	uint8_t		cn_random_6fold;
	uint8_t		dtxd_nodata_count;
	uint8_t		dtxd_sid_flag;
};

/* we use the same LFSR PRNG for CN as ETSI EFR implementation */
#define PN_INITIAL_SEED 0x70816958L   /* Pseudo noise generator seed value  */

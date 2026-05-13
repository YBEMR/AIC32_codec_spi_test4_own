/*
 * This header file is internal to libgsmfr2; it contains declarations
 * of internal functions for the Rx DTX preprocessor component.
 */

extern void gsmfr_preproc_gen_cn(struct gsmfr_preproc_state *state,
				 uint8_t *frame);
extern void gsmfr_preproc_sid2cn(struct gsmfr_preproc_state *state,
				 uint8_t *frame);
extern void gsmfr_preproc_invalid_sid(struct gsmfr_preproc_state *state,
				      uint8_t *frame);
extern uint16_t gsmfr_preproc_prng(struct gsmfr_preproc_state *state,
				   uint16_t no_bits);
extern uint8_t gsmfr_preproc_xmaxc_mean(const uint8_t *frame);

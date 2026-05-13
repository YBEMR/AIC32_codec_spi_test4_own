/*
 * This library-internal header file provides definition for
 * struct gsmhr_rxfe_state, the state structure for our Rx front end
 * that can function either as part of the full endpoint decoder
 * or standalone as a TFO transform.  The internal interface function
 * to the RxFE block as a whole (to be called from the full decoder
 * or from the TFO wrapper) is also declared here.
 */

#ifndef rxfe_h
#define rxfe_h

#include <stdint.h>
#include "tw_gsmhr.h"
#include "typedefs.h"

#define	GS_HISTORY_SIZE	28

struct gsmhr_rxfe_state {
	Shortword saved_frame[GSMHR_NUM_PARAMS];
	Longword gs_history[GS_HISTORY_SIZE];
	Shortword cn_r0_lpc[4];
	Longword cn_prng;
	Shortword gs_cn_out;
	uint8_t in_dtx;
	uint8_t ecu_state;
	uint8_t dtx_bfi_count;
	uint8_t dtx_muting;
	uint8_t gs_history_ptr;
};

void rxfe_main(struct gsmhr_rxfe_state *st, const Shortword *prm_in,
		Shortword *prm_out, int fast_cn_muting,
		Shortword *deco_mode_out, Shortword *mute_permit,
		Shortword *dtxd_sp);

#endif	/* include guard */

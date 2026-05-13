/*
 * This C module is the top level entity for our stateful decoder engine.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tw_amr.h"
#include "namespace.h"
#include "typedef.h"
#include "cnst.h"
#include "dec_amr.h"
#include "pstfilt.h"
#include "post_pro.h"
#include "bitno.h"

struct amr_decoder_state {
	Decoder_amrState	dec;
	Post_FilterState	pstfilt;
	Post_ProcessState	posthp;
	enum Mode		prev_mode;
	Flag			is_homed;
};

struct amr_decoder_state __decode_st;

struct amr_decoder_state *amr_decoder_create(void)
{
	struct amr_decoder_state *st;

	st = malloc(sizeof(struct amr_decoder_state));
	if (st)
		amr_decoder_reset(st);
	return st;
}

void amr_decoder_reset(struct amr_decoder_state *st)
{
	Decoder_amr_reset(&st->dec, 0);
	Post_Filter_reset(&st->pstfilt);
	Post_Process_reset(&st->posthp);
	st->prev_mode = (enum Mode) 0;
	st->is_homed = 1;
}

void amr_decode_frame(struct amr_decoder_state *st,
			const struct amr_param_frame *frame, int16_t *pcm)
{
	enum Mode mode;
	Word16 parm[MAX_PRM_SIZE];
	Word16 Az_dec[AZ_SIZE];
	Word16 i;

	/* fast home state handling needs to be first */
	if (st->is_homed && amr_check_dhf(frame, 1)) {
		for (i = 0; i < L_FRAME; i++)
			pcm[i] = EHF_MASK;
		return;
	}
	/* "unpack" into internal form */
	if (frame->type == RX_NO_DATA)
		mode = st->prev_mode;
	else {
		mode = frame->mode & 7;
		st->prev_mode = mode;
	}
	memcpy(parm, frame->param, prmno[mode] * sizeof(int16_t));
	/* now we can call the guts of the decoder */
	Decoder_amr(&st->dec, mode, parm, frame->type, pcm, Az_dec);
	Post_Filter(&st->pstfilt, mode, pcm, Az_dec);
	Post_Process(&st->posthp, pcm, L_FRAME);
	/*
	 * The 3 lsbs of each speech sample typically won't be all 0
	 * out of Post_Process(), hence we have to clear them explicitly
	 * to maintain overall bit-exact operation.
	 */
	for (i = 0; i < L_FRAME; i++)
		pcm[i] &= 0xFFF8;
	/* final check for full DHF */
	if (amr_check_dhf(frame, 0))
		amr_decoder_reset(st);
	else
		st->is_homed = 0;
}

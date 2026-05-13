/*
 * This C module is the top level entity for our stateful encoder engine.
 */

#include <stdint.h>
#include <stdlib.h>
#include "tw_amr.h"
#include "namespace.h"
#include "typedef.h"
#include "cnst.h"
#include "cod_amr.h"
#include "pre_proc.h"
#include "sid_sync.h"
#include "e_homing.h"

struct amr_encoder_state {
	cod_amrState		cod;
	Pre_ProcessState	pre;
	sid_syncState		sid;
};

struct amr_encoder_state __encode_st;

struct amr_encoder_state *amr_encoder_create(int dtx, int use_vad2)
{
	struct amr_encoder_state *st;

	st = malloc(sizeof(struct amr_encoder_state));
	if (st)
		amr_encoder_reset(st, dtx, use_vad2);
	return st;
}

void amr_encoder_reset(struct amr_encoder_state *st, int dtx, int use_vad2)
{
	cod_amr_reset(&st->cod, dtx, use_vad2);
	Pre_Process_reset(&st->pre);
	sid_sync_reset(&st->sid);
}

void amr_encode_frame(struct amr_encoder_state *st, enum Mode mode,
			const int16_t *pcm, struct amr_param_frame *frame)
{
	Word16 new_speech[L_FRAME], syn[L_FRAME];
	enum Mode used_mode;
	enum TXFrameType tx_type;
	Word16 i;

	/* input */
	for (i = 0; i < L_FRAME; i++)
		new_speech[i] = pcm[i] & 0xFFF8;
	Pre_Process(&st->pre, new_speech, L_FRAME);

	/* main process */
	cod_amr(&st->cod, mode, new_speech, frame->param, &used_mode, syn);
	sid_sync(&st->sid, used_mode, &tx_type);
	frame->type = tx_type;
	frame->mode = mode;

	/* encoder homing */
	if (encoder_homing_frame_test(pcm))
		amr_encoder_reset(st, st->cod.dtx, st->cod.vadSt.use_vad2);
}

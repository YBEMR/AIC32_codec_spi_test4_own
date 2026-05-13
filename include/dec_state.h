/*
 * In this header file we define our decoder state structure.
 * This decoder state structure is internal to libgsmefr,
 * NOT part of our external public interface!
 */
#include "dtx.h"

struct EFR_decoder_state {
	/* from agc.c */
	Word16 past_gain;
	/* from decoder.c */
	Word16 synth_buf[L_FRAME + M];
	Word16 reset_flag_old;
	/* from dec_12k2.c */
	Word16 old_exc[L_FRAME + PIT_MAX + L_INTERPOL];
	Word16 lsp_old[M];
	Word16 mem_syn[M];
	Word16 prev_bf;
	Word16 bf_state;
	/* from d_plsf_5.c */
	Word16 past_r2_q[M];
	Word16 past_lsf_q[M];
	Word16 lsf_p_CN[M];
	Word16 lsf_old_CN[M];
	Word16 lsf_new_CN[M];
	/* from d_gains.c */
	Word16 pbuf[5];
	Word16 past_gain_pit;
	Word16 prev_gp;
	Word16 gbuf[5];
	Word16 past_gain_code;
	Word16 prev_gc;
	Word16 gcode0_CN;
	Word16 gain_code_old_CN;
	Word16 gain_code_new_CN;
	Word16 gain_code_muting_CN;
	Word16 past_qua_en[4];
	Word16 pred[4];
	/* from dtx.c */
	Word16 rxdtx_ctrl;
	Word32 L_pn_seed_rx;
	Word16 rx_dtx_state;
	Word16 rxdtx_aver_period;
	Word16 rxdtx_N_elapsed;
	Word16 prev_SID_frames_lost;
	Word16 buf_p_rx;
	Word16 lsf_old_rx[DTX_HANGOVER][M];
	Word16 gain_code_old_rx[4 * DTX_HANGOVER];
	/* from dec_lag6.c */
	Word16 old_T0;
	/* from preemph.c */
	Word16 mem_pre;
	/* from pstfilt2.c */
	Word16 res2[L_SUBFR];
	Word16 mem_syn_pst[M];
	/* our own addition */
	Word32 L_pn_seed_nodata;
};

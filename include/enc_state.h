/*
 * In this header file we define our encoder state structure.
 * This encoder state structure is internal to libgsmefr,
 * NOT part of our external public interface!
 */

#include "dtx.h"

struct EFR_encoder_state {
	/* from coder.c */
	Word16 dtx_mode;
	/* from cod_12k2.c */
	Word16 old_speech[L_TOTAL];
	Word16 old_wsp[L_FRAME + PIT_MAX];
	Word16 old_exc[L_FRAME + PIT_MAX + L_INTERPOL];
	Word16 ai_zero[L_SUBFR + MP1];
	Word16 hvec[L_SUBFR * 2];
	Word16 lsp_old[M];
	Word16 lsp_old_q[M];
	Word16 mem_syn[M];
	Word16 mem_w0[M];
	Word16 mem_w[M];
	Word16 mem_err[M + L_SUBFR];
	/* from levinson.c */
	Word16 old_A[M + 1];
	/* from pre_proc.c */
	struct preproc_state {
		Word16 y2_hi;
		Word16 y2_lo;
		Word16 y1_hi;
		Word16 y1_lo;
		Word16 x0;
		Word16 x1;
	} pre_proc;
	/* from q_plsf_5.c */
	Word16 past_r2_q[M];
	Word16 lsf_p_CN[M];
	/* from q_gains.c */
	Word16 past_qua_en[4];
	Word16 pred[4];
	Word16 gcode0_CN;
	/* from dtx.c */
	Word16 txdtx_ctrl;
	Word16 CN_excitation_gain;
	Word32 L_pn_seed_tx;
	Word16 txdtx_hangover;
	Word16 txdtx_N_elapsed;
	Word16 old_CN_mem_tx[6];
	Word16 buf_p_tx;
	Word16 lsf_old_tx[DTX_HANGOVER][M];
	Word16 gain_code_old_tx[4 * DTX_HANGOVER];
	/* from vad.c */
	struct vad_state {
		Word16 rvad[9];
		Word16 scal_rvad;
		Pfloat thvad;
		Word32 L_sacf[27];
		Word32 L_sav0[36];
		Word16 pt_sacf;
		Word16 pt_sav0;
		Word32 L_lastdm;
		Word16 adaptcount;
		Word16 burstcount;
		Word16 hangcount;
		Word16 oldlagcount;
		Word16 veryoldlagcount;
		Word16 oldlag;
	} vad;
	Word16 ptch;
};

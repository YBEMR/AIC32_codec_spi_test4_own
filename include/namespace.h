/*
 * The code from ETSI consists of many separate modules and lots of little
 * functions; if we were to keep all those global function names untreated,
 * our library would cause horrible namespace pollution for any application
 * linking with it.  Our current solution: we include this header in all
 * internal modules, transforming the names of all internal functions
 * with intermodule linkage.
 */

#define	Overflow	EFR__Overflow
#define	Carry		EFR__Carry

#define	add		EFR__add
#define	sub		EFR__sub
#define	abs_s		EFR__abs_s
#define	shl		EFR__shl
#define	shr		EFR__shr
#define	mult		EFR__mult
#define	L_mult		EFR__L_mult
#define	negate		EFR__negate
#define	extract_h	EFR__extract_h
#define	extract_l	EFR__extract_l
#define	round		EFR__round
#define	L_mac		EFR__L_mac
#define	L_msu		EFR__L_msu
#define	L_macNs		EFR__L_macNs
#define	L_msuNs		EFR__L_msuNs
#define	L_add		EFR__L_add
#define	L_sub		EFR__L_sub
#define	L_add_c		EFR__L_add_c
#define	L_sub_c		EFR__L_sub_c
#define	L_negate	EFR__L_negate
#define	mult_r		EFR__mult_r
#define	L_shl		EFR__L_shl
#define	L_shr		EFR__L_shr
#define	shr_r		EFR__shr_r
#define	mac_r		EFR__mac_r
#define	msu_r		EFR__msu_r
#define	L_deposit_h	EFR__L_deposit_h
#define	L_deposit_l	EFR__L_deposit_l
#define	L_shr_r		EFR__L_shr_r
#define	L_abs		EFR__L_abs
#define	L_sat		EFR__L_sat
#define	norm_s		EFR__norm_s
#define	div_s		EFR__div_s
#define	norm_l		EFR__norm_l

#define	L_Extract	EFR__L_Extract
#define	L_Comp		EFR__L_Comp
#define	Mpy_32		EFR__Mpy_32
#define	Mpy_32_16	EFR__Mpy_32_16
#define	Div_32		EFR__Div_32

#define	Inv_sqrt	EFR__Inv_sqrt
#define	Log2		EFR__Log2
#define	Pow2		EFR__Pow2

#define	Init_Pre_Process	EFR__Init_Pre_Process
#define	Pre_Process		EFR__Pre_Process
#define	Autocorr		EFR__Autocorr
#define	Lag_window		EFR__Lag_window
#define	Levinson		EFR__Levinson
#define	Az_lsp			EFR__Az_lsp
#define	Lsp_Az			EFR__Lsp_Az
#define	Lsf_lsp			EFR__Lsf_lsp
#define	Lsp_lsf			EFR__Lsp_lsf
#define	Reorder_lsf		EFR__Reorder_lsf
#define	Weight_Fac		EFR__Weight_Fac
#define	Weight_Ai		EFR__Weight_Ai
#define	Residu			EFR__Residu
#define	Syn_filt		EFR__Syn_filt
#define	Convolve		EFR__Convolve
#define	agc			EFR__agc
#define	agc2			EFR__agc2
#define	preemphasis		EFR__preemphasis

#define	Init_Coder_12k2		EFR__Init_Coder_12k2
#define	Coder_12k2		EFR__Coder_12k2
#define	Init_Decoder_12k2	EFR__Init_Decoder_12k2
#define	Decoder_12k2		EFR__Decoder_12k2
#define	Init_Post_Filter	EFR__Init_Post_Filter
#define	Post_Filter		EFR__Post_Filter
#define	code_10i40_35bits	EFR__code_10i40_35bits
#define	dec_10i40_35bits	EFR__dec_10i40_35bits
#define	Dec_lag6		EFR__Dec_lag6
#define	d_gain_pitch		EFR__d_gain_pitch
#define	D_plsf_5		EFR__D_plsf_5
#define	Enc_lag6		EFR__Enc_lag6
#define	q_gain_pitch		EFR__q_gain_pitch
#define	q_gain_code		EFR__q_gain_code
#define	G_pitch			EFR__G_pitch
#define	G_code			EFR__G_code
#define	Interpol_6		EFR__Interpol_6
#define	Int_lpc			EFR__Int_lpc
#define	Int_lpc2		EFR__Int_lpc2
#define	Pitch_fr6		EFR__Pitch_fr6
#define	Pitch_ol		EFR__Pitch_ol
#define	Pred_lt_6		EFR__Pred_lt_6
#define	Q_plsf_5		EFR__Q_plsf_5

#define	decoder_homing_frame_test	EFR__decoder_homing_frame_test
#define	decoder_reset			EFR__decoder_reset
#define	encoder_homing_frame_test	EFR__encoder_homing_frame_test
#define	encoder_reset			EFR__encoder_reset

#define	reset_tx_dtx			EFR__reset_tx_dtx
#define	reset_rx_dtx			EFR__reset_rx_dtx
#define	tx_dtx				EFR__tx_dtx
#define	rx_dtx				EFR__rx_dtx
#define	CN_encoding			EFR__CN_encoding
#define	update_lsf_history		EFR__update_lsf_history
#define	update_lsf_p_CN			EFR__update_lsf_p_CN
#define	aver_lsf_history		EFR__aver_lsf_history
#define	update_gain_code_history_tx	EFR__update_gain_code_history_tx
#define	update_gain_code_history_rx	EFR__update_gain_code_history_rx
#define	compute_CN_excitation_gain	EFR__compute_CN_excitation_gain
#define	update_gcode0_CN		EFR__update_gcode0_CN
#define	aver_gain_code_history		EFR__aver_gain_code_history
#define	build_CN_code			EFR__build_CN_code
#define	pseudonoise			EFR__pseudonoise
#define	interpolate_CN_param		EFR__interpolate_CN_param
#define	interpolate_CN_lsf		EFR__interpolate_CN_lsf

#define	vad_reset			EFR__vad_reset
#define	vad_computation			EFR__vad_computation
#define	periodicity_update		EFR__periodicity_update

#define	mean_lsf	EFR__mean_lsf
#define	dico1_lsf	EFR__dico1_lsf
#define	dico2_lsf	EFR__dico2_lsf
#define	dico3_lsf	EFR__dico3_lsf
#define	dico4_lsf	EFR__dico4_lsf
#define	dico5_lsf	EFR__dico5_lsf

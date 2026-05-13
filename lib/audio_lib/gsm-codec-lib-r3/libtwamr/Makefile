OBJS=	a_refl.o agc.o autocorr.o az_lsp.o b_cn_cod.o basicop2.o bgnscd.o \
	bitno.o bits2prm.o c1035pf.o c2_11pf.o c2_9pf.o c3_14pf.o c4_17pf.o \
	c8_31pf.o c_g_aver.o calc_cor.o calc_en.o cbsearch.o cl_ltp.o cod_amr.o\
	convolve.o cor_h.o d1035pf.o d2_11pf.o d2_9pf.o d3_14pf.o d4_17pf.o \
	d8_31pf.o d_gain_c.o d_gain_p.o d_plsf.o d_plsf_3.o d_plsf_5.o \
	dec_amr.o dec_gain.o dec_lag3.o dec_lag6.o dec_main.o dhf_check.o \
	dhf_subst.o dhf_subst2.o dhf_tables.o dtx_dec.o dtx_enc.o e_homing.o \
	ec_gains.o enc_lag3.o enc_lag6.o enc_main.o ex_ctrl.o fr_type_conv.o \
	g_adapt.o g_code.o g_pitch.o gain_q.o gains_tab.o gc_pred.o gmed_n.o \
	graytab.o hp_max.o ietf_fo.o ietf_hdr.o ietf_in.o ietf_out.o \
	if1_bit_order.o if1_pack.o if1_unpack.o int_lpc.o int_lsf.o inter_36.o \
	inv_sqrt.o lag_wind.o levinson.o lflg_upd.o log2.o lpc.o lsfwt.o lsp.o \
	lsp_avg.o lsp_az.o lsp_lsf.o lsp_tab.o mac_32.o ol_ltp.o oper_32b.o \
	p_ol_wgh.o ph_disp.o pitch_fr.o pitch_ol.o post_pro.o pow2.o pre_big.o \
	pre_proc.o pred_lt.o preemph.o prm2bits.o prmno.o pstfilt.o q_gain_c.o \
	q_gain_p.o q_plsf.o q_plsf3_tab.o q_plsf5_tab.o q_plsf_3.o q_plsf_5.o \
	qgain475.o qgain795.o qua_gain.o qua_gain_tab.o r_fft.o reorder.o \
	residu.o s10_8pf.o set_sign.o sid_sync.o spreproc.o spstproc.o sqrt_l.o\
	syn_filt.o tls_flags.o ton_stab.o tseq_in.o tseq_out.o vad1.o vad2.o \
	vad_reset.o weight_a.o window.o
HDRS=	a_refl.h agc.h autocorr.h az_lsp.h b_cn_cod.h basic_op.h bgnscd.h \
	bitno.h bits2prm.h c1035pf.h c2_11pf.h c2_9pf.h c3_14pf.h c4_17pf.h \
	c8_31pf.h c_g_aver.h calc_cor.h calc_en.h cbsearch.h cl_ltp.h cnst.h \
	cnst_vad.h cod_amr.h convolve.h cor_h.h d1035pf.h d2_11pf.h d2_9pf.h \
	d3_14pf.h d4_17pf.h d8_31pf.h d_gain_c.h d_gain_p.h d_plsf.h dec_amr.h \
	dec_gain.h dec_lag3.h dec_lag6.h dtx_common.h dtx_dec.h dtx_enc.h \
	e_homing.h ec_gains.h enc_lag3.h enc_lag6.h ex_ctrl.h g_adapt.h \
	g_code.h g_pitch.h gain_q.h gains_tab.h gc_pred.h gmed_n.h graytab.h \
	hp_max.h if1_func.h int_defs.h int_lpc.h int_lsf.h inter_36.h \
	inv_sqrt.h lag_wind.h levinson.h log2.h lpc.h lsfwt.h lsp.h lsp_avg.h \
	lsp_az.h lsp_lsf.h lsp_tab.h mac_32.h memops.h namespace.h no_count.h \
	ol_ltp.h oper_32b.h p_ol_wgh.h ph_disp.h pitch_fr.h pitch_ol.h \
	post_pro.h pow2.h pre_big.h pre_proc.h pred_lt.h preemph.h prm2bits.h \
	pstfilt.h q_gain_c.h q_gain_p.h q_plsf.h q_plsf3_tab.h q_plsf5_tab.h \
	qgain475.h qgain795.h qua_gain.h qua_gain_tab.h reorder.h residu.h \
	s10_8pf.h set_sign.h sid_sync.h spreproc.h spstproc.h sqrt_l.h \
	syn_filt.h ton_stab.h tw_amr.h typedef.h vad.h vad1.h vad2.h weight_a.h\
	window.h
LIB=	libtwamr.a

include ../config.defs

all:	${LIB}

namespace.h:	namespace.awk namespace.list
	awk -f namespace.awk namespace.list > $@

${OBJS}:	${HDRS}

${LIB}:	${OBJS}
	ar rcu $@ ${OBJS}
	ranlib $@

install:
	mkdir -p ${DESTDIR}${includedir}
	install -c -m 444 tw_amr.h ${DESTDIR}${includedir}
	mkdir -p ${DESTDIR}${libdir}
	install -c -m 444 ${LIB} ${DESTDIR}${libdir}

clean:
	rm -f *.[oa] namespace.h errs

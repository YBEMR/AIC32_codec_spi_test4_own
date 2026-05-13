/*
 * The function implemented in this module is an aid for AMR-EFR interworking;
 * it is meant to be invoked directly after amr_encode_frame().  It checks
 * the encoder output for MR122 DHF and turns it into EFR DHF just like
 * amr_dhf_subst_efr(), but only if the input PCM frame was an EHF.  This
 * slightly more complicated logic is needed in order to replicate the observed
 * behavior of AMR-EFR speech encoder in the extant GSM network of T-Mobile USA.
 */

#include "tw_amr.h"
#include "namespace.h"
#include "e_homing.h"

void amr_dhf_subst_efr2(struct amr_param_frame *frame, const int16_t *pcm)
{
	if (encoder_homing_frame_test(pcm))
		amr_dhf_subst_efr(frame);
}

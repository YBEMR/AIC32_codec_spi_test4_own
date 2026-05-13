/*
 * The function implemented in this module is an aid for AMR-EFR interworking:
 * it is meant to be invoked directly after amr_encode_frame(), it checks
 * the encoder output for MR122 DHF, and if the generated frame exactly
 * matches MR122 DHF, it is changed to GSM-EFR DHF.
 */

#include <stdint.h>
#include <string.h>
#include "tw_amr.h"

void amr_dhf_subst_efr(struct amr_param_frame *frame)
{
	if (frame->type != TX_SPEECH_GOOD)
		return;
	if (frame->mode != MR122)
		return;
	if (memcmp(frame->param, amr_dhf_mr122, AMR_MAX_PRM * sizeof(int16_t)))
		return;
	memcpy(frame->param, amr_dhf_gsmefr, AMR_MAX_PRM * sizeof(int16_t));
}

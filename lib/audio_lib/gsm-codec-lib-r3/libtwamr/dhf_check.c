/*
 * This module implements the DHF checking function.
 */

#include <stdint.h>
#include <string.h>
#include "tw_amr.h"
#include "typedef.h"
#include "namespace.h"
#include "bitno.h"

static const Word16 * const dhf_per_mode[8] = {
	amr_dhf_mr475,
	amr_dhf_mr515,
	amr_dhf_mr59,
	amr_dhf_mr67,
	amr_dhf_mr74,
	amr_dhf_mr795,
	amr_dhf_mr102,
	amr_dhf_mr122
};

int amr_check_dhf(const struct amr_param_frame *frame, int first_sub_only)
{
	const Word16 *table;
	Word16 nparam;

	if (frame->type != RX_SPEECH_GOOD)
		return 0;
	if ((frame->mode & 0x87) == 0x87)
		table = amr_dhf_gsmefr;
	else
		table = dhf_per_mode[frame->mode & 7];
	if (first_sub_only)
		nparam = prmnofsf[frame->mode & 7];
	else
		nparam = prmno[frame->mode & 7];
	if (memcmp(frame->param, table, nparam * sizeof(int16_t)))
		return 0;
	else
		return 1;
}

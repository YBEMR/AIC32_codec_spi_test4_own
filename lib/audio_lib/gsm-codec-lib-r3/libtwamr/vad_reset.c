/*
 * The vad_reset() function implemented in this module is new with libtwamr,
 * i.e., it does not originate from 3GPP, even though it is styled after
 * 3GPP AMR code.  This function initializes our unified vadState structure,
 * which is a union of vadState1 and vadState2, plus a selection flag.
 */

#include "typedef.h"
#include "namespace.h"
#include "vad.h"

void vad_reset(vadState *st, Flag use_vad2)
{
	st->use_vad2 = use_vad2;
	if (st->use_vad2)
		vad2_reset(&st->u.v2);
	else
		vad1_reset(&st->u.v1);
}

/*
 * In this module we implement encoder output conversion to 3GPP
 * test sequence format.
 */

#include <stdint.h>
#include <string.h>
#include "tw_amr.h"
#include "namespace.h"
#include "typedef.h"
#include "cnst.h"
#include "prm2bits.h"

void amr_frame_to_tseq(const struct amr_param_frame *frame, uint16_t *cod)
{
	enum Mode pack_mode;

	cod[0] = frame->type;
	memset(cod + 1, 0, (AMR_COD_WORDS-1) * sizeof(uint16_t));
	switch (frame->type) {
	case TX_SPEECH_GOOD:
		pack_mode = frame->mode;
		cod[MAX_SERIAL_SIZE+1] = frame->mode;
		break;
	case TX_SID_FIRST:
	case TX_SID_UPDATE:
		pack_mode = MRDTX;
		cod[MAX_SERIAL_SIZE+1] = frame->mode;
		break;
	case TX_NO_DATA:
		pack_mode = MRDTX;
		cod[MAX_SERIAL_SIZE+1] = 0xFFFF;
		break;
	default:
		/* invalid usage, not allowed! */
		return;
	}
	Prm2bits(pack_mode, frame->param, (Word16 *)(cod + 1));
}

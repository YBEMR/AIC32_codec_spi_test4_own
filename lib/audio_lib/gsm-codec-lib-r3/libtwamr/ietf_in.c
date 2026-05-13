/*
 * The function implemented in this module is responsible for parsing
 * an incoming RFC 4867 payload and turning it into struct amr_param_frame
 * for decoding.
 */

#include <stdint.h>
#include "tw_amr.h"
#include "typedef.h"
#include "namespace.h"
#include "cnst.h"
#include "int_defs.h"
#include "if1_func.h"

int amr_frame_from_ietf(const uint8_t *bytes, struct amr_param_frame *frame)
{
	Word16 ft, q, sti;
	Word16 serial[MAX_SERIAL_SIZE];

	ft = (bytes[0] & 0x78) >> 3;
	q  = (bytes[0] & 0x04) >> 2;
	if (ft == AMR_FT_NODATA) {
		frame->type = RX_NO_DATA;
		frame->mode = 0xFF;
		return 0;
	}
	if (ft > MRDTX)
		return -1;
	if1_unpack_bytes(ft, bytes + 1, serial);
	Bits2prm(ft, serial, frame->param);
	if (ft != MRDTX) {
		frame->type = q ? RX_SPEECH_GOOD : RX_SPEECH_BAD;
		frame->mode = ft;
	} else {
		sti = (bytes[5] & 0x10) >> 4;
		frame->mode = 0;
		if (bytes[5] & 0x08)
			frame->mode |= 1;
		if (bytes[5] & 0x04)
			frame->mode |= 2;
		if (bytes[5] & 0x02)
			frame->mode |= 4;
		if (q)
			frame->type = sti ? RX_SID_UPDATE : RX_SID_FIRST;
		else
			frame->type = RX_SID_BAD;
	}
	return 0;
}

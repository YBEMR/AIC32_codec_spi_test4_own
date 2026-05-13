/*
 * The function implemented in this module is responsible for turning
 * struct amr_param_frame (encoder output) into an RFC 4867 payload.
 */

#include <stdint.h>
#include "tw_amr.h"
#include "typedef.h"
#include "namespace.h"
#include "cnst.h"
#include "int_defs.h"
#include "if1_func.h"

static const uint8_t total_bytes_per_ft[9] =
		{13, 14, 16, 18, 20, 21, 27, 32, 6};

static const uint8_t first_octet_per_ft[9] =
		{0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C, 0x44};

unsigned amr_frame_to_ietf(const struct amr_param_frame *frame, uint8_t *bytes)
{
	Word16 serial[MAX_SERIAL_SIZE];
	Word16 sti;
	enum Mode ft;

	switch (frame->type) {
	case TX_SPEECH_GOOD:
		ft = frame->mode;
		break;
	case TX_SID_FIRST:
		ft = MRDTX;
		sti = 0;
		break;
	case TX_SID_UPDATE:
		ft = MRDTX;
		sti = 1;
		break;
	case TX_NO_DATA:
	default:
		bytes[0] = 0x7C;
		return 1;
	}
	Prm2bits(ft, frame->param, serial);
	bytes[0] = first_octet_per_ft[ft];
	if1_pack_bytes(ft, serial, bytes + 1);
	if (ft == MRDTX) {
		if (sti)
			bytes[5] |= 0x10;
		if (frame->mode & 1)
			bytes[5] |= 0x08;
		if (frame->mode & 2)
			bytes[5] |= 0x04;
		if (frame->mode & 4)
			bytes[5] |= 0x02;
	}
	return total_bytes_per_ft[ft];
}

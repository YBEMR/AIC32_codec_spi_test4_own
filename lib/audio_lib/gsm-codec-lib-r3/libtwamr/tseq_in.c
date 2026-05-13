/*
 * In this module we implement decoder input conversion from 3GPP
 * test sequence format to our internal struct amr_param_frame.
 */

#include <stdint.h>
#include <string.h>
#include "tw_amr.h"
#include "namespace.h"
#include "typedef.h"
#include "cnst.h"
#include "bits2prm.h"

int amr_frame_from_tseq(const uint16_t *cod, int use_rxtype,
			struct amr_param_frame *frame)
{
	enum RXFrameType rx_type;
	int rc;

	if (use_rxtype) {
		if (cod[0] >= RX_N_FRAMETYPES)
			return -1;
		rx_type = cod[0];
	} else {
		rc = amr_txtype_to_rxtype(cod[0], &rx_type);
		if (rc < 0)
			return -1;
	}
	frame->type = rx_type;
	if (rx_type == RX_NO_DATA) {
		frame->mode = 0xFF;
		return 0;
	}
	if (cod[MAX_SERIAL_SIZE+1] > 7)
		return -2;
	frame->mode = cod[MAX_SERIAL_SIZE+1];
	switch (rx_type) {
	case RX_SPEECH_GOOD:
	case RX_SPEECH_DEGRADED:
	case RX_SPEECH_BAD:
		Bits2prm(frame->mode, cod + 1, frame->param);
		break;
	case RX_SID_UPDATE:
	case RX_SID_BAD:
		Bits2prm(MRDTX, cod + 1, frame->param);
		break;
	}
	return 0;
}

/*
 * The function implemented in this module converts from TXFrameType
 * to RXFrameType.  It is needed for decoding standard test sequence
 * .cod files, where TXFrameType is used in the file but RXFrameType
 * is needed for decoding.
 */

#include "tw_amr.h"

int amr_txtype_to_rxtype(enum TXFrameType tx_type, enum RXFrameType *rx_type)
{
	switch (tx_type) {
	case TX_SPEECH_GOOD:
		*rx_type = RX_SPEECH_GOOD;
		return 0;
	case TX_SPEECH_DEGRADED:
		*rx_type = RX_SPEECH_DEGRADED;
		return 0;
	case TX_SPEECH_BAD:
		*rx_type = RX_SPEECH_BAD;
		return 0;
	case TX_SID_FIRST:
		*rx_type = RX_SID_FIRST;
		return 0;
	case TX_SID_UPDATE:
		*rx_type = RX_SID_UPDATE;
		return 0;
	case TX_SID_BAD:
		*rx_type = RX_SID_BAD;
		return 0;
	case TX_ONSET:
		*rx_type = RX_ONSET;
		return 0;
	case TX_NO_DATA:
		*rx_type = RX_NO_DATA;
		return 0;
	default:
		return -1;
	}
}

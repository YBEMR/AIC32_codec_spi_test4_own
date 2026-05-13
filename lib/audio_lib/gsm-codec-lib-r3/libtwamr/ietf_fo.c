/*
 * The function implemented in this module groks the first octet of
 * an RFC 4867 payload and tells the calling application how many more
 * bytes need to be read, or if the frame type is invalid.
 */

#include <stdint.h>
#include "tw_amr.h"

static const uint8_t extra_bytes_per_ft[9] =
		{12, 13, 15, 17, 19, 20, 26, 31, 5};

int amr_ietf_grok_first_octet(uint8_t fo)
{
	uint8_t ft;

	ft = (fo & 0x78) >> 3;
	if (ft == AMR_FT_NODATA)
		return 0;
	if (ft > MRDTX)
		return -1;
	return extra_bytes_per_ft[ft];
}

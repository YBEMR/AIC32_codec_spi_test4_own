/*
 * In this module we implement our function for unpacking bits from AMR IF1
 * and reshuffling them into the codec's natural bit order.
 */

#include <stdint.h>
#include "tw_amr.h"
#include "typedef.h"
#include "namespace.h"
#include "int_defs.h"
#include "if1_func.h"

static inline Word16 msb_get_bit(const uint8_t *buf, Word16 bn)
{
	Word16 pos_byte = bn >> 3;
	Word16 pos_bit  = 7 - (bn & 7);

	return (buf[pos_byte] >> pos_bit) & 1;
}

static void unpack_bits(const uint8_t *if1_bytes, Word16 *codec_bits,
			Word16 nbits, const uint8_t *table)
{
	Word16 n, nb;

	for (n = 0; n < nbits; n++) {
		if (table)
			nb = table[n];
		else
			nb = n;
		codec_bits[nb] = msb_get_bit(if1_bytes, n);
	}
}

void if1_unpack_bytes(enum Mode mode, const uint8_t *if1_bytes,
		      Word16 *serial_bits)
{
	switch (mode) {
	case MR475:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_475, sort_475);
		return;
	case MR515:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_515, sort_515);
		return;
	case MR59:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_59, sort_59);
		return;
	case MR67:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_67, sort_67);
		return;
	case MR74:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_74, sort_74);
		return;
	case MR795:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_795, sort_795);
		return;
	case MR102:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_102, sort_102);
		return;
	case MR122:            
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_122, sort_122);
		return;
	case MRDTX:
		unpack_bits(if1_bytes, serial_bits, AMR_NBITS_SID,
				(const uint8_t *) 0);
		return;
	}
}

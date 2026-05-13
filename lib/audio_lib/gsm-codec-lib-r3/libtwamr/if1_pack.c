/*
 * In this module we implement our function for packing AMR codec bits
 * from an array containing them in the codec's natural bit order
 * into the octet form of AMR IF1 and RFC 4867.
 */

#include <stdint.h>
#include "tw_amr.h"
#include "typedef.h"
#include "namespace.h"
#include "int_defs.h"
#include "if1_func.h"

static inline void msb_set_bit(uint8_t *buf, Word16 bn, Word16 bit)
{
	Word16 pos_byte = bn >> 3;
	Word16 pos_bit  = 7 - (bn & 7);

	if (bit)
		buf[pos_byte] |= (1 << pos_bit);
	else
		buf[pos_byte] &= ~(1 << pos_bit);
}

static void pack_bits(uint8_t *if1_bytes, const Word16 *src_bits, Word16 nbits,
		      const uint8_t *table)
{
	Word16 n, nb;

	for (n = 0; n < nbits; n++) {
		if (table)
			nb = table[n];
		else
			nb = n;
		msb_set_bit(if1_bytes, n, src_bits[nb]);
	}
}

void if1_pack_bytes(enum Mode mode, const Word16 *serial_bits,
		    uint8_t *if1_bytes)
{
	switch (mode) {
	case MR475:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_475, sort_475);
		if1_bytes[11] &= 0xFE;
		return;
	case MR515:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_515, sort_515);
		if1_bytes[12] &= 0xFE;
		return;
	case MR59:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_59, sort_59);
		if1_bytes[14] &= 0xFC;
		return;
	case MR67:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_67, sort_67);
		if1_bytes[16] &= 0xFC;
		return;
	case MR74:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_74, sort_74);
		if1_bytes[18] &= 0xF0;
		return;
	case MR795:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_795, sort_795);
		if1_bytes[19] &= 0xFE;
		return;
	case MR102:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_102, sort_102);
		if1_bytes[25] &= 0xF0;
		return;
	case MR122:            
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_122, sort_122);
		if1_bytes[30] &= 0xF0;
		return;
	case MRDTX:
		pack_bits(if1_bytes, serial_bits, AMR_NBITS_SID,
				(const uint8_t *) 0);
		if1_bytes[4] &= 0xE0;
		return;
	}
}

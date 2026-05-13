/*
 * A cleaner reimplementation of AMR Prm2bits() function.
 */

#include "typedef.h"
#include "namespace.h"
#include "bitno.h"
#include "prm2bits.h"

void Prm2bits(enum Mode mode, const Word16 prm[], Word16 bits[])
{
	const Word16 *t = bitno[mode];
	unsigned nparam = prmno[mode];
	unsigned n, p, mask;
	Word16 *b = bits;

	for (n = 0; n < nparam; n++) {
		p = prm[n];
		mask = 1 << (*t++ - 1);
		for (; mask; mask >>= 1) {
			if (p & mask)
				*b++ = 1;
			else
				*b++ = 0;
		}
	}
}

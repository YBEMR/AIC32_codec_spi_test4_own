/*
 * A cleaner reimplementation of AMR Bits2prm() function.
 */

#include "typedef.h"
#include "namespace.h"
#include "bitno.h"
#include "bits2prm.h"

void Bits2prm(enum Mode mode, const Word16 bits[], Word16 prm[])
{
	const Word16 *p = bits;
	const Word16 *t = bitno[mode];
	unsigned nparam = prmno[mode];
	unsigned n, m, acc;

	for (n = 0; n < nparam; n++) {
		acc = 0;
		for (m = 0; m < *t; m++) {
			acc <<= 1;
			if (*p)
				acc |= 1;
			p++;
		}
		prm[n] = acc;
		t++;
	}
}

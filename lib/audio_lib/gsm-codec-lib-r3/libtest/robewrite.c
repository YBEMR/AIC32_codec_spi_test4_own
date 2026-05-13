/*
 * Here we implement our PCM write helper function for "robe" format.
 */

#include <stdio.h>
#include <stdint.h>
#include "robewrite.h"

void write_pcm_to_robe(FILE *outf, const int16_t *pcm)
{
	uint8_t bytes[320], *dp;
	int16_t samp;
	unsigned n;

	dp = bytes;
	for (n = 0; n < 160; n++) {
		samp = pcm[n];
		*dp++ = (samp >> 8) & 0xFF;
		*dp++ = samp & 0xFF;
	}
	fwrite(bytes, 2, 160, outf);
}

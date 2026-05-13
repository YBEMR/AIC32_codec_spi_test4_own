/*
 * Here we implement our PCM read helper function for "robe" format.
 */

#include <stdio.h>
#include <stdint.h>
#include "roberead.h"

int robe_get_pcm_block(FILE *inf, int16_t *pcm)
{
	uint8_t bytes[320], *dp;
	int cc, i;

	cc = fread(bytes, 1, 320, inf);
	cc >>= 1;
	dp = bytes;
	for (i = 0; i < cc; i++) {
		pcm[i] = (dp[0] << 8) | dp[1];
		dp += 2;
	}
	while (i < 160)
		pcm[i++] = 0;
	return cc;
}

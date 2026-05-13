/*
 * Here we implement our PCM write helper function.
 */

#include <stdint.h>
#include "wavwriter.h"
#include "pcmwrite.h"

void write_pcm_to_wav(void *wav, const int16_t *pcm)
{
	uint8_t bytes[320], *dp;
	int16_t samp;
	unsigned n;

	dp = bytes;
	for (n = 0; n < 160; n++) {
		samp = pcm[n];
		*dp++ = samp & 0xFF;
		*dp++ = (samp >> 8) & 0xFF;
	}
	wav_write_data(wav, bytes, 320);
}

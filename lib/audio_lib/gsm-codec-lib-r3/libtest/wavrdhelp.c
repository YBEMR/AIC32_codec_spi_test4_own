/*
 * In this module we implement our helper functions
 * on top of the basic WAV reader.
 */

#include <stdio.h>
#include <stdint.h>
#include "wavreader.h"
#include "wavrdhelp.h"

int wavrd_check_header(void *wav, const char *filename)
{
	int format, sampleRate, channels, bitsPerSample;

	if (!wav_get_header(wav, &format, &channels, &sampleRate,
	    &bitsPerSample, NULL)) {
		fprintf(stderr, "error: %s is not a valid WAV file\n",
			filename);
		return -1;
	}
	if (format != 1) {
		fprintf(stderr, "error: %s has unsupported WAV format %d\n",
			filename, format);
		return -1;
	}
	if (bitsPerSample != 16) {
		fprintf(stderr,
			"error: %s has unsupported WAV sample depth %d\n",
			filename, bitsPerSample);
		return -1;
	}
	if (channels != 1) {
		fprintf(stderr, "error: %s has %d channels, need 1\n",
			filename, channels);
		return -1;
	}
	if (sampleRate != 8000) {
		fprintf(stderr,
			"error: %s has %d Hz sample rate, need 8000 Hz\n",
			filename, sampleRate);
		return -1;
	}
	return 0;
}

int wavrd_get_pcm_block(void *wav, int16_t *pcm)
{
	uint8_t bytes[320], *dp;
	int cc, i;

	cc = wav_read_data(wav, bytes, 320);
	cc >>= 1;
	dp = bytes;
	for (i = 0; i < cc; i++) {
		pcm[i] = dp[0] | (dp[1] << 8);
		dp += 2;
	}
	while (i < 160)
		pcm[i++] = 0;
	return cc;
}

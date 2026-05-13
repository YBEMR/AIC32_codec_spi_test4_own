/*
 * This header file declares the interface to our helper function
 * for writing 16-bit linear PCM samples to WAV output file.
 */

extern void write_pcm_to_wav(void *wav, const int16_t *pcm);

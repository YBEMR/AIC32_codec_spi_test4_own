/*
 * This header file declares the interface to our helper function
 * for reading 16-bit linear PCM samples from a "robe" input file.
 */

extern int robe_get_pcm_block(FILE *inf, int16_t *pcmbuf);

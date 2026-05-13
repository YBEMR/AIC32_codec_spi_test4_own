/*
 * This header file declares the interface to our helper function
 * for writing 16-bit linear PCM samples to a "robe" output file.
 */

extern void write_pcm_to_robe(FILE *outf, const int16_t *pcm);

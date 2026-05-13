/*
 * We implement a couple of helper functions on top of the WAV reader
 * which we've lifted out of opencore-amrnb test code; this header file
 * provides prototype declarations for these functions.
 */

extern int wavrd_check_header(void *wav, const char *filename);
extern int wavrd_get_pcm_block(void *wav, int16_t *pcmbuf);

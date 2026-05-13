/*
 * The definitions contained in this header file originally lived in
 * dtx.c; they are being factored out into a new header file as part of
 * splitting of dtx.c code into common, encoder-only and decoder-only
 * parts.
 */

/* Inverse values of DTX hangover period and DTX hangover period + 1 */

#define INV_DTX_HANGOVER (0x7fff / DTX_HANGOVER)
#define INV_DTX_HANGOVER_P1 (0x7fff / (DTX_HANGOVER+1))

#define NB_PULSE 10 /* Number of pulses in fixed codebook excitation */

/* Constant DTX_ELAPSED_THRESHOLD is used as threshold for allowing
   SID frame updating without hangover period in case when elapsed
   time measured from previous SID update is below 24 */

#define DTX_ELAPSED_THRESHOLD (24 + DTX_HANGOVER - 1)

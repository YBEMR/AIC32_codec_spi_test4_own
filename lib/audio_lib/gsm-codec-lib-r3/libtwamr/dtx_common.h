/*
 * This header file contains some common definitions that have been
 * factored out of dtx_enc.h; the original code used the construct of
 * dtx_dec.h including dtx_enc.h, but in the opinion of this developer
 * it is better to factor out the common bits.
 */

#ifndef dtx_common_h
#define dtx_common_h
 
/*
********************************************************************************
*                         LOCAL VARIABLES AND TABLES
********************************************************************************
*/
#define DTX_HIST_SIZE 8
#define DTX_ELAPSED_FRAMES_THRESH (24 + 7 -1)
#define DTX_HANG_CONST 7             /* yields eight frames of SP HANGOVER  */

#endif

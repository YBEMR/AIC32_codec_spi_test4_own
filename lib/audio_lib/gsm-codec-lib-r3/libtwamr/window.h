/*
 * This header file holds extern declarations for the tables in window.c,
 * previously statics in window.tab include file, but now with intermodule
 * linkage.
 */

#ifndef	window_h
#define	window_h

#include "typedef.h"
#include "cnst.h"

extern const Word16 window_200_40[L_WINDOW];
extern const Word16 window_160_80[L_WINDOW];
extern const Word16 window_232_8[L_WINDOW];

#endif

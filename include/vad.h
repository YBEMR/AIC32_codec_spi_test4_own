/*
 * In the original 3GPP code, the selection between VAD1 and VAD2 is made
 * only at compile time.  In libtwamr we support run-time selection between
 * these two VAD algorithms for tinkering and investigation work; this
 * header file implements the logic that fits this run-time selection
 * into the existing code structure from 3GPP.
 */

#ifndef	vad_h
#define	vad_h

#include "typedef.h"
#include "vad1.h"
#include "vad2.h"

typedef struct {
    Flag    use_vad2;
    union {
        vadState1   v1;
        vadState2   v2;
    } u;
} vadState;

void vad_reset(vadState *st, Flag use_vad2);

#endif	/* include guard */


/*
 * This header file contains preprocessor definitions and extern
 * declarations for the tables which originally resided in gains.tab
 * and were multiply included in many source modules.
 */

#ifndef	gains_tab_h
#define	gains_tab_h

#include "typedef.h"

#define NB_QUA_PITCH 16

extern const Word16 qua_gain_pitch[NB_QUA_PITCH];

#define NB_QUA_CODE 32

extern const Word16 qua_gain_code[NB_QUA_CODE*3];

#endif

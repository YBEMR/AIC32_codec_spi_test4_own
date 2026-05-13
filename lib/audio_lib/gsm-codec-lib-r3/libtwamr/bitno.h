/*
 * This header file holds extern declarations for the tables
 * that have been moved from bitno.tab to bitno.c and prmno.c
 * with intermodule linkage.
 */

#ifndef	bitno_h
#define	bitno_h

#include "typedef.h"

extern const Word16 * const bitno[9];
extern const Word16 prmno[9];
extern const Word16 prmnofsf[8];

#endif

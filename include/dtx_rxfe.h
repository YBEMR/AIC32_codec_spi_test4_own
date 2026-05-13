/*
 * This header file provides declarations for functions and data objects
 * in dtx_rxfe.c: subset of DTX functions used by the Rx front end that
 * operates on codec parameters only.
 */

#ifndef dtx_rxfe_h
#define dtx_rxfe_h

#include "typedefs.h"

extern const LongwordRom ppLr_gsTable[4][32];

void avgGsHistQntz(const Longword pL_GsHistory[], Longword *pL_GsAvgd);
Shortword gsQuant(Longword L_GsIn, Shortword swVoicingMode);
Shortword getPnBits(int iBits, Longword *L_PnSeed);

#endif	/* include guard */

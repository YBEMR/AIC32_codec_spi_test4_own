#ifndef enc_out_order_h
#define enc_out_order_h

#include "typedefs.h"

void   fillBitAlloc(int iVoicing, int iR0, int *piVqIndeces,
                           int iSoftInterp, int *piLags,
                           int *piCodeWrdsA, int *piCodeWrdsB,
                           int *piGsp0s, Shortword swVadFlag,
                           Shortword swSP, Shortword *pswBAlloc);

#endif	/* include guard */

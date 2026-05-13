#ifndef __DTX_CONST
#define __DTX_CONST

#include "typedefs.h"

#define PN_INIT_SEED (Longword)0x1091988L       /* initial seed for Comfort
                                                 * noise pn-generator */

#define CNINTPER    12                 /* inperpolation period of CN
                                        * parameters */

#define SPEECH      1
#define CNIFIRSTSID 2
#define CNICONT     3
#define CNIBFI      4

#define VALIDSID    11
#define INVALIDSID  22
#define GOODSPEECH  33
#define UNUSABLE    44

#endif

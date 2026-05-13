/*******************************************************************
 *
 * typedef statements of types used in all half-rate GSM routines
 *
 ******************************************************************/

#ifndef __TYPEDEFS
#define __TYPEDEFS

#include <stdint.h>

#define LW_SIGN (int32_t)0x80000000       /* sign bit */
#define LW_MIN  (int32_t)0x80000000
#define LW_MAX  (int32_t)0x7fffffff

#define SW_SIGN (int16_t)0x8000           /* sign bit for Shortword type */
#define SW_MIN  (int16_t)0x8000           /* smallest Ram */
#define SW_MAX  (int16_t)0x7fff           /* largest Ram */

/* Definition of Types *
 ***********************/

typedef int32_t Longword;              /* 32 bit "accumulator" (L_*) */
typedef int16_t Shortword;             /* 16 bit "register"  (sw*) */
typedef int16_t ShortwordRom;          /* 16 bit ROM data    (sr*) */
typedef int32_t LongwordRom;           /* 32 bit ROM data    (L_r*)  */

struct NormSw
{                                      /* normalized Shortword fractional
                                        * number snr.man precedes snr.sh (the
                                        * shift count)i */
  Shortword man;                       /* "mantissa" stored in 16 bit
                                        * location */
  Shortword sh;                        /* the shift count, stored in 16 bit
                                        * location */
};

/* Global constants *
 ********************/

#define NP 10                          /* order of the lpc filter */
#define N_SUB 4                        /* number of subframes */
#define F_LEN 160                      /* number of samples in a frame */
#define S_LEN 40                       /* number of samples in a subframe */
#define A_LEN 170                      /* LPC analysis length */
#define OS_FCTR 6                      /* maximum LTP lag oversampling
                                        * factor */

#define OVERHANG 8                     /* vad parameter */

#endif

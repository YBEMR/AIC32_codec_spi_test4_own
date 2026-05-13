/* adaptation between stdint types and those used by ETSI/3GPP AMR code */

#ifndef	typedef_h
#define	typedef_h

#include <stdint.h>

typedef int16_t Word16;
typedef int32_t Word32;
typedef uint8_t Flag;
/* Struct for storing pseudo floating point exponent and mantissa */
struct _fp
{
    Word16 e;          /* exponent */
    Word16 m;          /* mantissa */
};

typedef struct _fp Pfloat;


#endif	/* include guard */

/*
 * The original code from ETSI uses its own Copy() and Set_zero()
 * functions, operating on Word16 elements.  Here we implement them
 * as static inline functions wrapping around memcpy and memset.
 */

#include <string.h>

static inline void Copy (
    const Word16 x[],  /* (i)  : input vector                               */
    Word16 y[],        /* (o)  : output vector                              */
    Word16 L           /* (i)  : vector length                              */
)
{
	memcpy(y, x, L * 2);
}

static inline void Set_zero (
    Word16 x[],        /* (o)  : vector to clear                            */
    Word16 L           /* (i)  : length of vector                           */
)
{
	memset(x, 0, L * 2);
}

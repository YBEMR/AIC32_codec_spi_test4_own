/*
 * This header file contains constant definitions and extern declarations
 * for tables that were originally in qua_gain.tab and qgain475.tab,
 * included in both decoder and encoder modules.
 */

#ifndef	qua_gain_tab_h
#define	qua_gain_tab_h

#include "typedef.h"

/* table used in 'high' rates: MR67 MR74 */
#define VQ_SIZE_HIGHRATES 128

extern const Word16 table_gain_highrates[VQ_SIZE_HIGHRATES*4];

/* table used in 'low' rates: MR475, MR515, MR59 */
#define VQ_SIZE_LOWRATES 64

extern const Word16 table_gain_lowrates[VQ_SIZE_LOWRATES*4];

/* table that was originally in qgain475.tab */

#define MR475_VQ_SIZE 256

extern const Word16 table_gain_MR475[MR475_VQ_SIZE*4];

#endif	/* include guard */

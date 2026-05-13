/*
 * This module holds the bitno[] table (intermodule linkage)
 * and the set of static per-mode tables referenced from it,
 * originally in bitno.tab in 3GPP reference source.
 */

#include "typedef.h"
#include "namespace.h"
#include "int_defs.h"

/* parameter sizes (# of bits), one table per mode */

static const Word16 bitno_MR475[PRMNO_MR475] = {
   8, 8, 7,                                 /* LSP VQ          */
   8, 7, 2, 8,                              /* first subframe  */
   4, 7, 2,                                 /* second subframe */
   4, 7, 2, 8,                              /* third subframe  */
   4, 7, 2,                                 /* fourth subframe */
};

static const Word16 bitno_MR515[PRMNO_MR515] = {
   8, 8, 7,                                 /* LSP VQ          */
   8, 7, 2, 6,                              /* first subframe  */
   4, 7, 2, 6,                              /* second subframe */
   4, 7, 2, 6,                              /* third subframe  */
   4, 7, 2, 6,                              /* fourth subframe */
};

static const Word16 bitno_MR59[PRMNO_MR59] = {
   8, 9, 9,                                 /* LSP VQ          */
   8, 9, 2, 6,                              /* first subframe  */
   4, 9, 2, 6,                              /* second subframe */
   8, 9, 2, 6,                              /* third subframe  */
   4, 9, 2, 6,                              /* fourth subframe */
};

static const Word16 bitno_MR67[PRMNO_MR67] = {
   8, 9, 9,                                 /* LSP VQ          */
   8, 11, 3, 7,                             /* first subframe  */
   4, 11, 3, 7,                             /* second subframe */
   8, 11, 3, 7,                             /* third subframe  */
   4, 11, 3, 7,                             /* fourth subframe */
};

static const Word16 bitno_MR74[PRMNO_MR74] = {
   8, 9, 9,                                 /* LSP VQ          */
   8, 13, 4, 7,                             /* first subframe  */
   5, 13, 4, 7,                             /* second subframe */
   8, 13, 4, 7,                             /* third subframe  */
   5, 13, 4, 7,                             /* fourth subframe */
};

static const Word16 bitno_MR795[PRMNO_MR795] = {
   9, 9, 9,                                 /* LSP VQ          */
   8, 13, 4, 4, 5,                          /* first subframe  */
   6, 13, 4, 4, 5,                          /* second subframe */
   8, 13, 4, 4, 5,                          /* third subframe  */
   6, 13, 4, 4, 5,                          /* fourth subframe */
};

static const Word16 bitno_MR102[PRMNO_MR102] = {
   8, 9, 9,                                 /* LSP VQ          */
   8, 1, 1, 1, 1, 10, 10, 7, 7,             /* first subframe  */
   5, 1, 1, 1, 1, 10, 10, 7, 7,             /* second subframe */
   8, 1, 1, 1, 1, 10, 10, 7, 7,             /* third subframe  */
   5, 1, 1, 1, 1, 10, 10, 7, 7,             /* fourth subframe */
};

static const Word16 bitno_MR122[PRMNO_MR122] = {
   7, 8, 9, 8, 6,                           /* LSP VQ          */
   9, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5,   /* first subframe  */
   6, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5,   /* second subframe */
   9, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5,   /* third subframe  */
   6, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5    /* fourth subframe */
};

static const Word16 bitno_MRDTX[PRMNO_MRDTX] = {
  3,
  8, 9, 9,
  6
};

/* overall table with all parameter sizes for all modes */
const Word16 * const bitno[9] = {
   bitno_MR475,
   bitno_MR515,
   bitno_MR59,
   bitno_MR67,
   bitno_MR74,
   bitno_MR795,
   bitno_MR102,
   bitno_MR122,
   bitno_MRDTX
};

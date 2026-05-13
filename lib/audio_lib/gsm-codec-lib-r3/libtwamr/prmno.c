/*
 * This module holds prmno[] and prmnofs[] tables that were
 * originally in bitno.tab in 3GPP reference source.
 */

#include "typedef.h"
#include "namespace.h"
#include "int_defs.h"

/* number of parameters per modes (values must be <= MAX_PRM_SIZE!) */
const Word16 prmno[9] = {
  PRMNO_MR475,
  PRMNO_MR515,
  PRMNO_MR59,
  PRMNO_MR67,
  PRMNO_MR74,
  PRMNO_MR795,
  PRMNO_MR102,
  PRMNO_MR122,
  PRMNO_MRDTX
};

/* number of parameters to first subframe per modes */
const Word16 prmnofsf[8] = {
  PRMNOFSF_MR475,
  PRMNOFSF_MR515,
  PRMNOFSF_MR59,
  PRMNOFSF_MR67,
  PRMNOFSF_MR74,
  PRMNOFSF_MR795,
  PRMNOFSF_MR102,
  PRMNOFSF_MR122
};

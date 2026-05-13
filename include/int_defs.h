/*
 * This header file is internal to libtwamr (not public API);
 * it contains internal definitions for aspects of the AMR codec
 * that don't need to be in the external API.
 */

#ifndef	int_defs_h
#define	int_defs_h

/* number of speech bits for all modes */
#define	AMR_NBITS_475		95
#define	AMR_NBITS_515		103
#define	AMR_NBITS_59		118
#define	AMR_NBITS_67		134
#define	AMR_NBITS_74		148
#define	AMR_NBITS_795		159
#define	AMR_NBITS_102		204
#define	AMR_NBITS_122		244
#define	AMR_NBITS_SID		35

/* number of distinct parameters for all modes */
#define	PRMNO_MR475		17
#define	PRMNO_MR515		19
#define	PRMNO_MR59		19
#define	PRMNO_MR67		19
#define	PRMNO_MR74		19
#define	PRMNO_MR795		23
#define	PRMNO_MR102		39
#define	PRMNO_MR122		57
#define	PRMNO_MRDTX		5

/* number of parameters up to first subframe (for DHF detection) */
#define	PRMNOFSF_MR475		7
#define	PRMNOFSF_MR515		7
#define	PRMNOFSF_MR59		7
#define	PRMNOFSF_MR67		7
#define	PRMNOFSF_MR74		7
#define	PRMNOFSF_MR795		8
#define	PRMNOFSF_MR102		12
#define	PRMNOFSF_MR122		18

#endif	/* include guard */

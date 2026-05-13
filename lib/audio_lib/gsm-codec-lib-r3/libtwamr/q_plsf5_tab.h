/*
 * The original ETSI EFR code features an include file named q_plsf_5.tab,
 * included into d_homing.c, d_plsf_5.c and q_plsf_5.c, duplicating
 * the small mean_lsf[] table across all 3 modules and the other (big)
 * tables across d_plsf_5.c for the decoder and q_plsf_5.c for the encoder.
 *
 * In our version we have moved the tables into q_plsf5_tab.c,
 * with extern declarations in q_plsf5_tab.h.
 *
 * Update for libtwamr: this table is unchanged from ETSI EFR, hence
 * we are keeping our version unchanged from libgsmefr too.
 */

#ifndef	q_plsf5_tab_h
#define	q_plsf5_tab_h

#include "typedef.h"

extern const Word16 mean_lsf[10];

#define DICO1_SIZE  128
#define DICO2_SIZE  256
#define DICO3_SIZE  256
#define DICO4_SIZE  256
#define DICO5_SIZE  64

extern const Word16 dico1_lsf[DICO1_SIZE * 4];
extern const Word16 dico2_lsf[DICO2_SIZE * 4];
extern const Word16 dico3_lsf[DICO3_SIZE * 4];
extern const Word16 dico4_lsf[DICO4_SIZE * 4];
extern const Word16 dico5_lsf[DICO5_SIZE * 4];

#endif

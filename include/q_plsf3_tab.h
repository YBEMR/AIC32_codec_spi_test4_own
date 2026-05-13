/*
 * In ETSI EFR code there was only q_plsf_5.tab that was problematically
 * multi-included in many source files; in the AMR version there is also
 * q_plsf_3.tab with a similar situation.  Handle it similarly.
 */

#ifndef	q_plsf3_tab_h
#define	q_plsf3_tab_h

#include "typedef.h"

#define PAST_RQ_INIT_SIZE 8

extern const Word16 past_rq_init[80];
extern const Word16 mean_lsf3[10];
extern const Word16 pred_fac[10];

#define DICO31_SIZE  256
#define DICO32_SIZE  512
#define DICO33_SIZE  512

extern const Word16 dico1_lsf3[];
extern const Word16 dico2_lsf3[];
extern const Word16 dico3_lsf3[];

#define MR515_3_SIZE  128

extern const Word16 mr515_3_lsf[];

#define MR795_1_SIZE  512

extern const Word16 mr795_1_lsf[];

#endif

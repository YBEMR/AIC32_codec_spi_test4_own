/*
********************************************************************************
*
*      GSM AMR-NB speech codec   R98   Version 7.6.0   December 12, 2001
*                                R99   Version 3.3.0                
*                                REL-4 Version 4.1.0                
*
********************************************************************************
*
*      File             : q_plsf.h
*      Purpose          : common include file for LSF vector/matrix quanti-
*                         sation modules (q_plsf_3.c/q_plsf_5.c)
*
********************************************************************************
*/
#ifndef q_plsf_h
#define q_plsf_h "$Id $"
 
/*
********************************************************************************
*                         INCLUDE FILES
********************************************************************************
*/
#include "tw_amr.h"
#include "typedef.h"
#include "cnst.h"
/*
********************************************************************************
*                         DEFINITION OF DATA TYPES
********************************************************************************
*/
typedef struct {
    Word16 past_rq[M];    /* Past quantized prediction error, Q15 */
} Q_plsfState;
 
/*
********************************************************************************
*                         DECLARATION OF PROTOTYPES
********************************************************************************
*/

void Q_plsf_reset (Q_plsfState *st);
/* reset of state (i.e. set state memory to zero)
   returns 0 on success
 */

void Q_plsf_3(
    Q_plsfState *st,    /* i/o: state struct                             */
    enum Mode mode,     /* i  : coder mode                               */
    Word16 *lsp1,       /* i  : 1st LSP vector                      Q15  */
    Word16 *lsp1_q,     /* o  : quantized 1st LSP vector            Q15  */
    Word16 *indice,     /* o  : quantization indices of 3 vectors   Q0   */
    Word16 *pred_init_i /* o  : init index for MA prediction in DTX mode */
);

void Q_plsf_5 (
    Q_plsfState *st,
    Word16 *lsp1,      /* i  : 1st LSP vector,                     Q15 */
    Word16 *lsp2,      /* i  : 2nd LSP vector,                     Q15 */   
    Word16 *lsp1_q,    /* o  : quantized 1st LSP vector,           Q15 */
    Word16 *lsp2_q,    /* o  : quantized 2nd LSP vector,           Q15 */
    Word16 *indice     /* o  : quantization indices of 5 matrices, Q0  */
);
 
#endif

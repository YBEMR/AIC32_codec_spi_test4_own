/*
*****************************************************************************
*
*      GSM AMR-NB speech codec   R98   Version 7.6.0   December 12, 2001
*                                R99   Version 3.3.0                
*                                REL-4 Version 4.1.0                
*
*****************************************************************************
*
*      File             : lsp_avg.c
*      Purpose:         : LSP averaging and history
*
*****************************************************************************
*/

/*
*****************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*****************************************************************************
*/
#include "namespace.h"
#include "lsp_avg.h"
 
/*
*****************************************************************************
*                         INCLUDE FILES
*****************************************************************************
*/
#include "basic_op.h"
#include "oper_32b.h"
#include "no_count.h"
#include "q_plsf5_tab.h"
#include "memops.h"

/*
*****************************************************************************
*                         LOCAL VARIABLES AND TABLES
*****************************************************************************
*/
/*
*****************************************************************************
*                         PUBLIC PROGRAM CODE
*****************************************************************************
*/
 
/*
**************************************************************************
*
*  Function    : lsp_avg_reset
*  Purpose     : Resets state memory
*
**************************************************************************
*/
void lsp_avg_reset (lsp_avgState *st)
{ 
  Copy(mean_lsf, &st->lsp_meanSave[0], M);
}
 
/*
**************************************************************************
*
*  Function    : lsp_avg
*  Purpose     : Calculate the LSP averages
*
**************************************************************************
*/

void lsp_avg (
    lsp_avgState *st,         /* i/o : State struct                 Q15 */
    Word16 *lsp               /* i   : state of the state machine   Q15 */
)
{
    Word16 i;
    Word32 L_tmp;            /* Q31 */

    for (i = 0; i < M; i++) {

       /* mean = 0.84*mean */
       L_tmp = L_deposit_h(st->lsp_meanSave[i]);
       L_tmp = L_msu(L_tmp, EXPCONST, st->lsp_meanSave[i]);

       /* Add 0.16 of newest LSPs to mean */
       L_tmp = L_mac(L_tmp, EXPCONST, lsp[i]);

       /* Save means */
       st->lsp_meanSave[i] = round(L_tmp);          move16();   /* Q15 */
    }

    return;
}

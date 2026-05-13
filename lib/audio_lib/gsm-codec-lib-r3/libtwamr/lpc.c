/*
********************************************************************************
*
*      GSM AMR-NB speech codec   R98   Version 7.6.0   December 12, 2001
*                                R99   Version 3.3.0                
*                                REL-4 Version 4.1.0                
*
********************************************************************************
*
*      File             : lpc.c
*
********************************************************************************
*/

/*
********************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
********************************************************************************
*/
#include "namespace.h"
#include "lpc.h"

/*
********************************************************************************
*                         INCLUDE FILES
********************************************************************************
*/
#include "tw_amr.h"
#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "autocorr.h"
#include "lag_wind.h"
#include "levinson.h"
#include "cnst.h"
#include "no_count.h"
#include "window.h"

/*
********************************************************************************
*                         PUBLIC PROGRAM CODE
********************************************************************************
*/
 
/*************************************************************************
*
*  Function:   lpc_reset
*
**************************************************************************
*/
void lpc_reset (lpcState *state)
{
  Levinson_reset(&state->levinsonSt);
}

int lpc(
    lpcState *st,     /* i/o: State struct                */
    enum Mode mode,   /* i  : coder mode                  */
    Word16 x[],       /* i  : Input signal           Q15  */
    Word16 x_12k2[],  /* i  : Input signal (EFR)     Q15  */
    Word16 a[]        /* o  : predictor coefficients Q12  */
)
{
   Word16 rc[4];                  /* First 4 reflection coefficients Q15 */
   Word16 rLow[MP1];
   Word16 rHigh[MP1];  /* Autocorrelations low and hi      */
                                  /* No fixed Q value but normalized  */
                                  /* so that overflow is avoided      */
   
   test ();
   if ( sub (mode, MR122) == 0)
   {
       /* Autocorrelations */
       Autocorr(x_12k2, M, rHigh, rLow, window_160_80);              
       /* Lag windowing    */
       Lag_window(M, rHigh, rLow);                                   
       /* Levinson Durbin  */
       Levinson(&st->levinsonSt, rHigh, rLow, &a[MP1], rc);

       /* Autocorrelations */
       Autocorr(x_12k2, M, rHigh, rLow, window_232_8);                  
       /* Lag windowing    */
       Lag_window(M, rHigh, rLow);                                  
       /* Levinson Durbin  */
       Levinson(&st->levinsonSt, rHigh, rLow, &a[MP1 * 3], rc);
   }
   else
   {
       /* Autocorrelations */
       Autocorr(x, M, rHigh, rLow, window_200_40);                 
       /* Lag windowing    */
       Lag_window(M, rHigh, rLow);                                   
       /* Levinson Durbin  */
       Levinson(&st->levinsonSt, rHigh, rLow, &a[MP1 * 3], rc);
   }

   return 0;
}

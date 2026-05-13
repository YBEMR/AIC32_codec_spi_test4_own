/*
 * This header file has been adapted from inc/private.h
 * in TU-Berlin libgsm source, original notice follows:
 *
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

#define	SASR(x, by)	((x) >> (by))

/*
 *	Prototypes from add.c
 */
extern word	gsm_mult 	(word a, word b);
extern longword gsm_L_mult 	(word a, word b);
extern word	gsm_mult_r	(word a, word b);

extern word	gsm_div  	(word num, word denum);

extern word	gsm_add 	( word a, word b );
extern longword gsm_L_add 	( longword a, longword b );

extern word	gsm_sub 	(word a, word b);
extern longword gsm_L_sub 	(longword a, longword b);

extern word	gsm_abs 	(word a);

extern word	gsm_norm 	( longword a );

extern longword gsm_L_asl  	(longword a, int n);
extern word	gsm_asl 	(word a, int n);

extern longword gsm_L_asr  	(longword a, int n);
extern word	gsm_asr  	(word a, int n);

/*
 *  Inlined functions from add.h
 */

/*
 * #define GSM_MULT_R(a, b) (* word a, word b, !(a == b == MIN_WORD) *)	\
 *	(0x0FFFF & SASR(((longword)(a) * (longword)(b) + 16384), 15))
 */
#define GSM_MULT_R(a, b) /* word a, word b, !(a == b == MIN_WORD) */	\
	(SASR( ((longword)(a) * (longword)(b) + 16384), 15 ))

# define GSM_MULT(a,b)	 /* word a, word b, !(a == b == MIN_WORD) */	\
	(SASR( ((longword)(a) * (longword)(b)), 15 ))

# define GSM_L_MULT(a, b) /* word a, word b */	\
	(((longword)(a) * (longword)(b)) << 1)

# define GSM_L_ADD(a, b)	\
	( (a) <  0 ? ( (b) >= 0 ? (a) + (b)	\
		 : (utmp = (ulongword)-((a) + 1) + (ulongword)-((b) + 1)) \
		   >= MAX_LONGWORD ? MIN_LONGWORD : -(longword)utmp-2 )   \
	: ((b) <= 0 ? (a) + (b)   \
	          : (utmp = (ulongword)(a) + (ulongword)(b)) >= MAX_LONGWORD \
		    ? MAX_LONGWORD : utmp))

/*
 * # define GSM_ADD(a, b)	\
 * 	((ltmp = (longword)(a) + (longword)(b)) >= MAX_WORD \
 * 	? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)
 */
/* Nonportable, but faster: */

#define	GSM_ADD(a, b)	\
	((ulongword)((ltmp = (longword)(a) + (longword)(b)) - MIN_WORD) > \
		MAX_WORD - MIN_WORD ? (ltmp > 0 ? MAX_WORD : MIN_WORD) : ltmp)

# define GSM_SUB(a, b)	\
	((ltmp = (longword)(a) - (longword)(b)) >= MAX_WORD \
	? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)

# define GSM_ABS(a)	((a) < 0 ? ((a) == MIN_WORD ? MAX_WORD : -(a)) : (a))

/* Use these if necessary:

# define GSM_MULT_R(a, b)	gsm_mult_r(a, b)
# define GSM_MULT(a, b)		gsm_mult(a, b)
# define GSM_L_MULT(a, b)	gsm_L_mult(a, b)

# define GSM_L_ADD(a, b)	gsm_L_add(a, b)
# define GSM_ADD(a, b)		gsm_add(a, b)
# define GSM_SUB(a, b)		gsm_sub(a, b)

# define GSM_ABS(a)		gsm_abs(a)

*/

/*
 *  More prototypes from implementations..
 */

void Gsm_Long_Term_Predictor (			/* 4x for 160 samples */
		struct gsmfr_0610_state * S,
		word	* d,	/* [0..39]   residual signal	IN	*/
		word	* dp,	/* [-120..-1] d'		IN	*/
		word	* e,	/* [0..40] 			OUT	*/
		word	* dpp,	/* [0..40] 			OUT	*/
		word	* Nc,	/* correlation lag		OUT	*/
		word	* bc	/* gain factor			OUT	*/);

void Gsm_LPC_Analysis (
		struct gsmfr_0610_state * S,
		word * s,	/* 0..159 signals	IN/OUT	*/
		word * LARc);	/* 0..7   LARc's	OUT	*/

void Gsm_Preprocess (
		struct gsmfr_0610_state * S,
		const word * s, word * so);

void Gsm_Encoding (
		struct gsmfr_0610_state * S,
		word	* e,	
		word	* ep,	
		word	* xmaxc,
		word	* Mc,	
		word	* xMc);

void Gsm_Short_Term_Analysis_Filter (
		struct gsmfr_0610_state * S,
		const word * LARc, /* coded log area ratio [0..7]  IN	*/
		word	* d	/* st res. signal [0..159]	IN/OUT	*/);

void Gsm_Decoding (
		struct gsmfr_0610_state * S,
		word 	xmaxcr,
		word	Mcr,
		const word * xMcr,	/* [0..12]		IN	*/
		word	* erp); 	/* [0..39]		OUT 	*/

void Gsm_Long_Term_Synthesis_Filtering (
		struct gsmfr_0610_state* S,
		word	Ncr,
		word	bcr,
		word	* erp,		/* [0..39]		  IN 	*/
		word	* drp); 	/* [-120..-1] IN, [0..40] OUT 	*/

void Gsm_RPE_Decoding (
		struct gsmfr_0610_state *S,
		word xmaxcr,
		word Mcr,
		const word * xMcr,	/* [0..12], 3 bits        IN      */
		word * erp);		/* [0..39]                OUT     */

void Gsm_RPE_Encoding (
		struct gsmfr_0610_state * S,
		word    * e,		/* -5..-1][0..39][40..44     IN/OUT  */
		word    * xmaxc,	/*                              OUT */
		word    * Mc,		/*                              OUT */
		word    * xMc);		/* [0..12]                      OUT */

void Gsm_Short_Term_Synthesis_Filter (
		struct gsmfr_0610_state * S,
		const word * LARcr,	/* log area ratios [0..7]  IN	*/
		word	* drp,		/* received d [0...39]	   IN	*/
		word	* s);		/* signal   s [0..159]	  OUT	*/

void Gsm_Update_of_reconstructed_short_time_residual_signal (
		word	* dpp,		/* [0...39]	IN	*/
		word	* ep,		/* [0...39]	IN	*/
		word	* dp);		/* [-120...-1]  IN/OUT 	*/

/*
 *  Tables from table.c
 */

extern word gsm_A[8], gsm_B[8], gsm_MIC[8], gsm_MAC[8];
extern word gsm_INVA[8];
extern word gsm_DLB[4], gsm_QLB[4];
extern word gsm_H[11];
extern word gsm_NRFAC[8];
extern word gsm_FAC[8];

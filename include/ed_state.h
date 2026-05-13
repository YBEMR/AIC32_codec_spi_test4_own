/*
 * This header file is internal to libgsmfr2;
 * here we define our state structure for GSM 06.10 encoder & decoder component.
 */

struct gsmfr_0610_state {
	word		dp0[ 280 ];
	word		e[ 50 ];	/* code.c 			*/

	word		z1;		/* preprocessing.c, Offset_com. */
	longword	L_z2;		/*                  Offset_com. */
	int		mp;		/*                  Preemphasis	*/

	word		u[8];		/* short_term_aly_filter.c	*/
	word		LARpp[2][8]; 	/*                              */
	word		j;		/*                              */

	word		nrp; /* 40 */	/* long_term.c, synthesis	*/
	word		v[9];		/* short_term.c, synthesis	*/
	word		msr;		/* decoder.c,	Postprocessing	*/
};

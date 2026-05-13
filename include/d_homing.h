/**************************************************************************
 *
 *   File Name:  d_homing.h
 *
 *   Purpose:   Contains the prototypes for all the functions of
 *              decoder homing.
 *
 **************************************************************************/

#define EHF_MASK 0x0008 /* Encoder Homing Frame pattern */

/* Function Prototypes */

Word16 decoder_homing_frame_test (const Word16 parm[], Word16 nbr_of_params);

void decoder_reset (struct EFR_decoder_state *st);

/*
// TI File $Revision: /main/9 $
// Checkin $Date: August 28, 2007   11:23:38 $
//###########################################################################
//
// FILE:	F28335.cmd
//
// TITLE:	Linker Command File For F28335 Device
//
//###########################################################################
// $TI Release: DSP2833x Header Files V1.01 $
// $Release Date: September 26, 2007 $
//###########################################################################
*/

/* ======================================================
// For Code Composer Studio V2.2 and later
// ---------------------------------------
// In addition to this memory linker command file, 
// add the header linker command file directly to the project. 
// The header linker command file is required to link the
// peripheral structures to the proper locations within 
// the memory map.
//
// The header linker files are found in <base>\DSP2833x_Headers\cmd
//   
// For BIOS applications add:      DSP2833x_Headers_BIOS.cmd
// For nonBIOS applications add:   DSP2833x_Headers_nonBIOS.cmd    
========================================================= */

/* ======================================================
// For Code Composer Studio prior to V2.2
// --------------------------------------
// 1) Use one of the following -l statements to include the 
// header linker command file in the project. The header linker
// file is required to link the peripheral structures to the proper 
// locations within the memory map                                    */

/* Uncomment this line to include file only for non-BIOS applications */
/* -l DSP2833x_Headers_nonBIOS.cmd */

/* Uncomment this line to include file only for BIOS applications */
/* -l DSP2833x_Headers_BIOS.cmd */

/* 2) In your project add the path to <base>\DSP2833x_headers\cmd to the
   library search path under project->build options, linker tab, 
   library search path (-i).
/*========================================================= */

/* Define the memory block start/length for the F28335  
   PAGE 0 will be used to organize program sections
   PAGE 1 will be used to organize data sections

    Notes: 
          Memory blocks on F28335 are uniform (ie same
          physical memory) in both PAGE 0 and PAGE 1.  
          That is the same memory region should not be
          defined for both PAGE 0 and PAGE 1.
          Doing so will result in corruption of program 
          and/or data. 
          
          L0/L1/L2 and L3 memory blocks are mirrored - that is
          they can be accessed in high memory or low memory.
          For simplicity only one instance is used in this
          linker file. 
          
          Contiguous SARAM memory blocks can be combined 
          if required to create a larger memory block. 
 */


MEMORY
{
PAGE 0:    /* Program Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE1 for data allocation */

   ZONE0       : origin = 0x004000, length = 0x001000     /* XINTF zone 0 */
   RAML0       : origin = 0x008000, length = 0x001000     /* on-chip RAM block L0 */
   //RAML1       : origin = 0x009000, length = 0x001000     /* on-chip RAM block L1 */
   //RAML2       : origin = 0x00A000, length = 0x001000     /* on-chip RAM block L2 */
   //RAML3       : origin = 0x00B000, length = 0x001000     /* on-chip RAM block L3 */
   ZONE6A      : origin = 0x100000, length = 0x00FC00    /* XINTF zone 6 - program space*/ 
   ZONE7FAST   : origin = 0x236000, length = 0x00A000    /* XINTF zone 7 - fast code */
   //ZONE7       : origin = 0x200000, length = 0x100000    /* XINTF zone 7  */
   FLASHG      : origin = 0x308000, length = 0x008000     /* on-chip FLASH */
   FLASHF      : origin = 0x310000, length = 0x008000     /* on-chip FLASH */
   FLASHE      : origin = 0x318000, length = 0x008000     /* on-chip FLASH */
   FLASHD      : origin = 0x320000, length = 0x007E80     /* on-chip FLASH */
   BEGIN       : origin = 0x327E80, length = 0x000002  /* keep app entry outside bootloader sector */
   //BEGIN       : origin = 0x33FFF6, length = 0x000002
   FLASHC      : origin = 0x328000, length = 0x008000     /* on-chip FLASH */
   CSM_RSVD    : origin = 0x33FF80, length = 0x000076     /* Part of FLASHA.  Program with all 0x0000 when CSM is in use. */
   CSM_PWL     : origin = 0x33FFF8, length = 0x000008     /* Part of FLASHA.  CSM password locations in FLASHA */
   FASTLOAD    : origin = 0x330000, length = 0x008000     /* FLASHB only; keep FLASHA/CSM untouched */
   OTP         : origin = 0x380400, length = 0x000400     /* on-chip OTP */
   ADC_CAL     : origin = 0x380080, length = 0x000009     /* ADC_cal function in Reserved memory */
   
   IQTABLES    : origin = 0x3FE000, length = 0x000b50     /* IQ Math Tables in Boot ROM */
   IQTABLES2   : origin = 0x3FEB50, length = 0x00008c     /* IQ Math Tables in Boot ROM */  
   FPUTABLES   : origin = 0x3FEBDC, length = 0x0006A0     /* FPU Tables in Boot ROM */
   ROM         : origin = 0x3FF27C, length = 0x000D44     /* Boot ROM */        
   RESET       : origin = 0x3FFFC0, length = 0x000002     /* part of boot ROM  */
   VECTORS     : origin = 0x3FFFC2, length = 0x00003E     /* part of boot ROM  */

PAGE 1 :   /* Data Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE0 for program allocation */
           /* Registers remain on PAGE1                                                  */
   
   BOOT_RSVD   : origin = 0x000000, length = 0x000050     /* Part of M0, BOOT rom will use this for stack */
   RAMM0       : origin = 0x000050, length = 0x0003B0     /* on-chip RAM block M0 */
   RAMM1       : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */

   RAML1       : origin = 0x009000, length = 0x006000     /* on-chip RAM block L1 */

   //RAML4       : origin = 0x00C000, length = 0x001000     /* on-chip RAM block L1 */
   //RAML5       : origin = 0x00D000, length = 0x001000     /* on-chip RAM block L1 */
   //RAML6       : origin = 0x00E000, length = 0x001000     /* on-chip RAM block L1 */
   RAML7       : origin = 0x00F000, length = 0x001000     /* on-chip RAM block L1 */
   ZONE6B      : origin = 0x10FC00, length = 0x000400     /* XINTF zone 6 - data space */
   ZONE7DATA   : origin = 0x200000, length = 0x036000
}

/* Allocate sections to memory blocks.
   Note:
         codestart user defined section in DSP28_CodeStartBranch.asm used to redirect code 
                   execution when booting to flash
         ramfuncs  user defined section to store functions that will be copied from Flash into RAM
*/ 
 
SECTIONS
{
 
   /* Allocate program areas: */
   .cinit              : > FLASHC      PAGE = 0
   .pinit              : > FLASHC,     PAGE = 0
   codestart           : > BEGIN       PAGE = 0
   ramfuncs            :
                         {
                            *(ramfuncs)
                            /*
                             * MR515 encode hot path experiment groups.
                             * Keep cod_amr.obj in Flash because it mostly dispatches work.
                             *
                             * Active experiment: Open-loop pitch group.
                             * Profiling after LTP-A showed open_loop was the largest
                             * stage, about 14 ms/frame, so this replaces LTP-A in RAML0.
                            */
                            audio_lib.lib<pitch_ol.obj>(.text)
                            audio_lib.lib<hp_max.obj>(.text)
                            audio_lib.lib<ol_ltp.obj>(.text)
                            audio_lib.lib<pre_big.obj>(.text)
                            /*
                             * p_ol_wgh.obj is part of the open-loop family but
                             * does not fit with pitch_ol + hp_max in RAML0.
                             *
                             * audio_lib.lib<p_ol_wgh.obj>(.text)
                             */

                            /*
                             * LTP-A: closed-loop pitch search baseline.
                             * Best measured so far: about 50 ms/frame.
                             *
                             * audio_lib.lib<pitch_fr.obj>(.text)
                             * audio_lib.lib<cl_ltp.obj>(.text)
                             * audio_lib.lib<pred_lt.obj>(.text)
                             * audio_lib.lib<convolve.obj>(.text)
                             */

                            /*
                             * LTP-B: replace pitch_fr with pitch gain math.
                             *
                             * audio_lib.lib<g_pitch.obj>(.text)
                             * audio_lib.lib<cl_ltp.obj>(.text)
                             * audio_lib.lib<pred_lt.obj>(.text)
                             * audio_lib.lib<convolve.obj>(.text)
                             */

                            /*
                             * Codebook group: fixed codebook correlation/sign search.
                             * This measured about 53 ms/frame when tested alone.
                             *
                             * audio_lib.lib<cor_h.obj>(.text)
                             * audio_lib.lib<set_sign.obj>(.text)
                             */

                            /*
                             * Gain group: gain quantization plus compact LTP helpers.
                             * This group is designed to fit RAML0 by keeping pitch_fr
                             * in Flash and retaining the smaller LTP helpers.
                             *
                             * audio_lib.lib<gain_q.obj>(.text)
                             * audio_lib.lib<qua_gain.obj>(.text)
                             * audio_lib.lib<cl_ltp.obj>(.text)
                             * audio_lib.lib<pred_lt.obj>(.text)
                             * audio_lib.lib<convolve.obj>(.text)
                             */
                         } LOAD = FLASHD,
                           RUN = RAML0,
                           LOAD_START(_RamfuncsLoadStart),
                           LOAD_END(_RamfuncsLoadEnd),
                           RUN_START(_RamfuncsRunStart),
                           PAGE = 0

   amrfastcode         :
                         {
                           /*
                            * External RAM is used only for cold or non-MR515 code.
                            * Do not place the current MR515 hot path here because it
                            * contends with ZONE7DATA on the XINTF bus.
                            */
                           audio_lib.lib<s10_8pf.obj>(.text)
                           audio_lib.lib<c4_17pf.obj>(.text)
                           audio_lib.lib<qgain475.obj>(.text)
                           audio_lib.lib<c3_14pf.obj>(.text)
                           audio_lib.lib<c8_31pf.obj>(.text)
                           audio_lib.lib<c2_11pf.obj>(.text)
                           /*
                           audio_lib.lib<pitch_fr.obj>(.text)
                           audio_lib.lib<g_pitch.obj>(.text)
                           audio_lib.lib<gc_pred.obj>(.text)
                           audio_lib.lib<cor_h.obj>(.text)
                           audio_lib.lib<set_sign.obj>(.text)
                           audio_lib.lib<pitch_ol.obj>(.text)
                           audio_lib.lib<c2_9pf.obj>(.text)
                           audio_lib.lib<qua_gain.obj>(.text)
                           audio_lib.lib<g_code.obj>(.text)
                           audio_lib.lib<hp_max.obj>(.text)
                           audio_lib.lib<autocorr.obj>(.text)
                           audio_lib.lib<az_lsp.obj>(.text)
                           */
                         } LOAD = FASTLOAD,
                           RUN = ZONE7FAST,
                           LOAD_START(_AmrFastLoadStart),
                           LOAD_END(_AmrFastLoadEnd),
                           RUN_START(_AmrFastRunStart),
                           PAGE = 0

   amrfastcode2        :
                         {
                           /*
                           audio_lib.lib<dtx_enc.obj>(.text)
                           audio_lib.lib<c1035pf.obj>(.text)
                           audio_lib.lib<p_ol_wgh.obj>(.text)
                           audio_lib.lib<ph_disp.obj>(.text)
                           audio_lib.lib<bgnscd.obj>(.text)

                           
                           audio_lib.lib<q_plsf_5.obj>(.text)
                           audio_lib.lib<q_plsf_3.obj>(.text)
                           audio_lib.lib<r_fft.obj>(.text)
                           audio_lib.lib<levinson.obj>(.text)
                           audio_lib.lib<gain_q.obj>(.text)
                           audio_lib.lib<cl_ltp.obj>(.text)
                           audio_lib.lib<pre_proc.obj>(.text)
                           audio_lib.lib<pred_lt.obj>(.text)
                           audio_lib.lib<convolve.obj>(.text)
                           audio_lib.lib<pre_big.obj>(.text)
                           */
                            
                         } LOAD = FLASHD,
                           RUN = ZONE7FAST,
                           LOAD_START(_AmrFast2LoadStart),
                           LOAD_END(_AmrFast2LoadEnd),
                           RUN_START(_AmrFast2RunStart),
                           PAGE = 0

   .text               : >> FLASHG | FLASHF | FLASHE | FLASHD   PAGE = 0

   csmpasswds          : > CSM_PWL     PAGE = 0, TYPE = DSECT
   csm_rsvd            : > CSM_RSVD    PAGE = 0, TYPE = DSECT
   
   /* Allocate uninitalized data sections: */
   .stack              : > RAML1       PAGE = 1
   .ebss               : > RAML7       PAGE = 1
   .esysmem            : > RAMM1       PAGE = 1

   /* Initalized sections go in Flash */
   /* For SDFlash to program these, they must be allocated to page 0 */
   .econst             : > FLASHE      PAGE = 0
   .switch             : > FLASHE      PAGE = 0

   /* Allocate IQ math areas: */
   IQmath              : > FLASHC      PAGE = 0                  /* Math Code */
   IQmathTables     : > IQTABLES,  PAGE = 0, TYPE = NOLOAD 
   IQmathTables2    : > IQTABLES2, PAGE = 0, TYPE = NOLOAD 
   FPUmathTables    : > FPUTABLES, PAGE = 0, TYPE = NOLOAD 
         
   /* Allocate DMA-accessible RAM sections: */
   //DMARAML4         : > RAML4,     PAGE = 1
   //DMARAML5         : > RAML5,     PAGE = 1
   //DMARAML6         : > RAML6,     PAGE = 1
   //DMARAML7         : > RAML7,     PAGE = 1
   
   /* Allocate 0x400 of XINTF Zone 6 to storing data */
   ZONE6DATA        : > ZONE6B,    PAGE = 1
   ZONE7DATA        : > ZONE7DATA,    PAGE = 1

   /* .reset is a standard section used by the compiler.  It contains the */ 
   /* the address of the start of _c_int00 for C Code.   /*
   /* When using the boot ROM this section and the CPU vector */
   /* table is not needed.  Thus the default type is set here to  */
   /* DSECT  */ 
   .reset              : > RESET,      PAGE = 0, TYPE = DSECT
   vectors             : > VECTORS     PAGE = 0, TYPE = DSECT
   
   /* Allocate ADC_cal function (pre-programmed by factory into TI reserved memory) */
   .adc_cal     : load = ADC_CAL,   PAGE = 0, TYPE = NOLOAD

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/


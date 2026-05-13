/*
 * This module holds const data tables of all supported DHFs:
 * 8 per-mode DHFs for AMR plus the different DHF for GSM-EFR.
 */

#include "tw_amr.h"

const int16_t amr_dhf_mr475[AMR_MAX_PRM] =
{
    0x00F8,
    0x009D,
    0x001C,
    0x0066,
    0x0000,
    0x0003,
    0x0028,
    0x000F,
    0x0038,
    0x0001,
    0x000F,
    0x0031,
    0x0002,
    0x0008,
    0x000F,
    0x0026,
    0x0003
};

const int16_t amr_dhf_mr515[AMR_MAX_PRM] =
{
    0x00F8,
    0x009D,
    0x001C,
    0x0066,
    0x0000,
    0x0003,
    0x0037,
    0x000F,
    0x0000,
    0x0003,
    0x0005,
    0x000F,
    0x0037,
    0x0003,
    0x0037,
    0x000F,
    0x0023,
    0x0003,
    0x001F
};

const int16_t amr_dhf_mr59[AMR_MAX_PRM] =
{
    0x00F8,
    0x00E3,
    0x002F,
    0x00BD,
    0x0000,
    0x0003,
    0x0037,
    0x000F,
    0x0001,
    0x0003,
    0x000F,
    0x0060,
    0x00F9,
    0x0003,
    0x0037,
    0x000F,
    0x0000,
    0x0003,
    0x0037
};

const int16_t amr_dhf_mr67[AMR_MAX_PRM] =
{
    0x00F8,
    0x00E3,
    0x002F,
    0x00BD,
    0x0002,
    0x0007,
    0x0000,
    0x000F,
    0x0098,
    0x0007,
    0x0061,
    0x0060,
    0x05C5,
    0x0007,
    0x0000,
    0x000F,
    0x0318,
    0x0007,
    0x0000
};

const int16_t amr_dhf_mr74[AMR_MAX_PRM] =
{
    0x00F8,
    0x00E3,
    0x002F,
    0x00BD,
    0x0006,
    0x000F,
    0x0000,
    0x001B,
    0x0208,
    0x000F,
    0x0062,
    0x0060,
    0x1BA6,
    0x000F,
    0x0000,
    0x001B,
    0x0006,
    0x000F,
    0x0000
};

const int16_t amr_dhf_mr795[AMR_MAX_PRM] =
{
    0x00C2,
    0x00E3,
    0x002F,
    0x00BD,
    0x0006,
    0x000F,
    0x000A,
    0x0000,
    0x0039,
    0x1C08,
    0x0007,
    0x000A,
    0x000B,
    0x0063,
    0x11A6,
    0x000F,
    0x0001,
    0x0000,
    0x0039,
    0x09A0,
    0x000F,
    0x0002,
    0x0001
};

const int16_t amr_dhf_mr102[AMR_MAX_PRM] =
{
    0x00F8,
    0x00E3,
    0x002F,
    0x0045,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x001B,
    0x0000,
    0x0001,
    0x0000,
    0x0001,
    0x0326,
    0x00CE,
    0x007E,
    0x0051,
    0x0062,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x015A,
    0x0359,
    0x0076,
    0x0000,
    0x001B,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x017C,
    0x0215,
    0x0038,
    0x0030
};

const int16_t amr_dhf_mr122[AMR_MAX_PRM] =
{
    0x0004,
    0x002A,
    0x00DB,
    0x0096,
    0x002A,
    0x0156,
    0x000B,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0036,
    0x000B,
    0x0000,
    0x000F,
    0x000E,
    0x000C,
    0x000D,
    0x0000,
    0x0001,
    0x0005,
    0x0007,
    0x0001,
    0x0008,
    0x0024,
    0x0000,
    0x0001,
    0x0000,
    0x0005,
    0x0006,
    0x0001,
    0x0002,
    0x0004,
    0x0007,
    0x0004,
    0x0002,
    0x0003,
    0x0036,
    0x000B,
    0x0000,
    0x0002,
    0x0004,
    0x0000,
    0x0003,
    0x0006,
    0x0001,
    0x0007,
    0x0006,
    0x0005,
    0x0000
};

const int16_t amr_dhf_gsmefr[AMR_MAX_PRM] =
{
        0x0004,                 /* LPC 1 */
        0x002f,                 /* LPC 2 */
        0x00b4,                 /* LPC 3 */
        0x0090,                 /* LPC 4 */
        0x003e,                 /* LPC 5 */

        0x0156,                 /* LTP-LAG 1 */
        0x000b,                 /* LTP-GAIN 1 */
        0x0000,                 /* PULSE 1_1 */
        0x0001,                 /* PULSE 1_2 */
        0x000f,                 /* PULSE 1_3 */
        0x0001,                 /* PULSE 1_4 */
        0x000d,                 /* PULSE 1_5 */
        0x0000,                 /* PULSE 1_6 */
        0x0003,                 /* PULSE 1_7 */
        0x0000,                 /* PULSE 1_8 */
        0x0003,                 /* PULSE 1_9 */
        0x0000,                 /* PULSE 1_10 */
        0x0003,                 /* FCB-GAIN 1 */

        0x0036,                 /* LTP-LAG 2 */
        0x0001,                 /* LTP-GAIN 2 */
        0x0008,                 /* PULSE 2_1 */
        0x0008,                 /* PULSE 2_2 */
        0x0005,                 /* PULSE 2_3 */
        0x0008,                 /* PULSE 2_4 */
        0x0001,                 /* PULSE 2_5 */
        0x0000,                 /* PULSE 2_6 */
        0x0000,                 /* PULSE 2_7 */
        0x0001,                 /* PULSE 2_8 */
        0x0001,                 /* PULSE 2_9 */
        0x0000,                 /* PULSE 2_10 */
        0x0000,                 /* FCB-GAIN 2 */

        0x0156,                 /* LTP-LAG 3 */
        0x0000,                 /* LTP-GAIN 3 */
        0x0000,                 /* PULSE 3_1 */
        0x0000,                 /* PULSE 3_2 */
        0x0000,                 /* PULSE 3_3 */
        0x0000,                 /* PULSE 3_4 */
        0x0000,                 /* PULSE 3_5 */
        0x0000,                 /* PULSE 3_6 */
        0x0000,                 /* PULSE 3_7 */
        0x0000,                 /* PULSE 3_8 */
        0x0000,                 /* PULSE 3_9 */
        0x0000,                 /* PULSE 3_10 */
        0x0000,                 /* FCB-GAIN 3 */

        0x0036,                 /* LTP-LAG 4 */
        0x000b,                 /* LTP-GAIN 4 */
        0x0000,                 /* PULSE 4_1 */
        0x0000,                 /* PULSE 4_2 */
        0x0000,                 /* PULSE 4_3 */
        0x0000,                 /* PULSE 4_4 */
        0x0000,                 /* PULSE 4_5 */
        0x0000,                 /* PULSE 4_6 */
        0x0000,                 /* PULSE 4_7 */
        0x0000,                 /* PULSE 4_8 */
        0x0000,                 /* PULSE 4_9 */
        0x0000,                 /* PULSE 4_10 */
        0x0000                  /* FCB-GAIN 4 */
};

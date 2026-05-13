/*
 * This header file provides prototype declarations for AMR IF1
 * packing and unpacking functions, as well as extern declarations
 * for the set of const data tables they need.
 */

#ifndef	if1_func_h
#define	if1_func_h

#include <stdint.h>
#include "tw_amr.h"
#include "typedef.h"

void if1_pack_bytes(enum Mode mode, const Word16 *serial_bits,
		    uint8_t *if1_bytes);
void if1_unpack_bytes(enum Mode mode, const uint8_t *if1_bytes,
		      Word16 *serial_bits);

extern const uint8_t sort_475[95];
extern const uint8_t sort_515[103];
extern const uint8_t sort_59[118];
extern const uint8_t sort_67[134];
extern const uint8_t sort_74[148];
extern const uint8_t sort_795[159];
extern const uint8_t sort_102[204];
extern const uint8_t sort_122[244];

#endif	/* include guard */

/*
 * Here we implement our binfile_read_frame() function.
 */

#include <stdio.h>
#include <stdint.h>
#include "binreader.h"

int binfile_read_frame(FILE *binf, uint8_t *frame)
{
	int cc, morelen;

	cc = fread(frame, 1, 1, binf);
	if (cc != 1)
		return 0;
	if (frame[0] == 0xBF)
		morelen = 1;
	else if ((frame[0] & 0xF0) == 0xC0)
		morelen = 30;
	else if ((frame[0] & 0xF0) == 0xD0)
		morelen = 32;
	else
		return -1;
	cc = fread(frame+1, 1, morelen, binf);
	if (cc == morelen)
		return 1;
	else
		return -2;
}

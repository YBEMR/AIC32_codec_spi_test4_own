/*
 * All TCH/F downlink capture files produced by fc-shell tch record
 * (both FR and EFR, both old and new formats) include the same
 * 81-character block in each line: 3 DSP status words followed by
 * 33 bytes of frame data.  Here we are factoring out the function
 * for parsing this common part.
 */

#include <ctype.h>
#include <stdint.h>

static
decode_hex_digit(ch)
{
	if (isdigit(ch))
		return(ch - '0');
	else if (isupper(ch))
		return(ch - 'A' + 10);
	else
		return(ch - 'a' + 10);
}

parse_dlcap_common(line, status_words, tidsp_bytes)
	char *line;
	uint16_t *status_words;
	uint8_t *tidsp_bytes;
{
	char *cp;
	int i;

	/* grok DSP status words */
	cp = line;
	for (i = 0; i < 3; i++) {
		if (!isxdigit(cp[0]) || !isxdigit(cp[1]) ||
		    !isxdigit(cp[2]) || !isxdigit(cp[3]))
			return -1;
		status_words[i] = (decode_hex_digit(cp[0]) << 12) |
				  (decode_hex_digit(cp[1]) << 8) |
				  (decode_hex_digit(cp[2]) << 4) |
				   decode_hex_digit(cp[3]);
		cp += 4;
		if (*cp++ != ' ')
			return -1;
	}
	/* read the frame bits */
	for (i = 0; i < 33; i++) {
		if (!isxdigit(cp[0]) || !isxdigit(cp[1]))
			return -1;
		tidsp_bytes[i] = (decode_hex_digit(cp[0]) << 4) |
				  decode_hex_digit(cp[1]);
		cp += 2;
	}
	return 0;
}

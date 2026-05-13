/*
 * This header file declares the interface to our writer function for
 * hexadecimal RTP frame sequence files in TW-TS-005 format.  It is
 * a simple function that emits an array of bytes as a hexadecimal line.
 */

void emit_hex_frame(FILE *outf, const uint8_t *frame, unsigned nbytes);

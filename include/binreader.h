/*
 * This header file defines the interface to our reader function for binary
 * files recording GSM FR or EFR streams, see ../doc/Binary-file-format.
 *
 * binfile_read_frame() return values are:
 *  1 = successfully read valid frame
 *  0 = normal EOF
 * -1 = unrecognized header byte
 * -2 = EOF in the middle of a frame
 */

#define	BINFILE_MAX_FRAME	33

extern int binfile_read_frame(FILE *binf, uint8_t *frame);

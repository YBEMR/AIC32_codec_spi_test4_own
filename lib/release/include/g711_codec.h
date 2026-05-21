#ifndef G711_CODEC_H
#define G711_CODEC_H

#include "DSP2833x_Device.h"

/*
 * G.711 A-law for 8 kHz narrowband voice.
 *
 * 20 ms frame:
 *   PCM input:  160 samples, signed 16-bit linear PCM
 *   A-law out:  160 octets
 *
 * On TMS320F28335 / C28x:
 *   We store each 8-bit G.711 octet in one Uint16.
 *   Only the low 8 bits are valid.
 */

#define G711_FRAME_SAMPLES      160U
#define G711_FRAME_OCTETS       160U

#define G711_OCTET_MASK         0x00FFU

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Encode one signed 16-bit PCM sample to one G.711 A-law octet.
 * Return value:
 *   low 8 bits contain valid A-law code.
 */
Uint16 G711A_LinearToAlaw(int16 linear);

/*
 * Decode one G.711 A-law octet to one signed 16-bit PCM sample.
 * Input:
 *   only low 8 bits of alaw are used.
 */
int16 G711A_AlawToLinear(Uint16 alaw);

/*
 * Encode a PCM frame.
 *
 * pcm_in:
 *   signed 16-bit PCM samples.
 *
 * alaw_out:
 *   each Uint16 stores one 8-bit A-law octet in low 8 bits.
 *
 * sample_count:
 *   normally 160 for 20 ms @ 8 kHz.
 */
void G711A_EncodeFrame(const int16 *pcm_in,
                       Uint16 *alaw_out,
                       Uint16 sample_count);

/*
 * Decode an A-law frame.
 *
 * alaw_in:
 *   each Uint16 stores one 8-bit A-law octet in low 8 bits.
 *
 * pcm_out:
 *   signed 16-bit PCM output samples.
 *
 * octet_count:
 *   normally 160 for 20 ms @ 8 kHz.
 */
void G711A_DecodeFrame(const Uint16 *alaw_in,
                       int16 *pcm_out,
                       Uint16 octet_count);

#ifdef __cplusplus
}
#endif

#endif

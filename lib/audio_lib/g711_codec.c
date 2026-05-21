#include "g711_codec.h"

/*
 * A-law uses alternating mark inversion mask.
 * This comes from the SpanDSP/WebRTC implementation.
 */
#define G711_ALAW_AMI_MASK      0x55U

/*
 * Find the bit position of the highest set bit.
 *
 * The original WebRTC/SpanDSP code uses unsigned int and 32-bit masks.
 * On C28x, int is usually 16-bit, so here we explicitly use Uint32.
 *
 * Return:
 *   highest set bit index, or -1 if bits == 0.
 */
static int16 G711_TopBit(Uint32 bits)
{
    int16 i;

    if (bits == 0UL)
    {
        return -1;
    }

    i = 0;

    if ((bits & 0xFFFF0000UL) != 0UL)
    {
        bits &= 0xFFFF0000UL;
        i += 16;
    }

    if ((bits & 0xFF00FF00UL) != 0UL)
    {
        bits &= 0xFF00FF00UL;
        i += 8;
    }

    if ((bits & 0xF0F0F0F0UL) != 0UL)
    {
        bits &= 0xF0F0F0F0UL;
        i += 4;
    }

    if ((bits & 0xCCCCCCCCUL) != 0UL)
    {
        bits &= 0xCCCCCCCCUL;
        i += 2;
    }

    if ((bits & 0xAAAAAAAAUL) != 0UL)
    {
        i += 1;
    }

    return i;
}

/*
 * PCM16 linear sample -> G.711 A-law octet.
 *
 * This is adapted from the WebRTC/SpanDSP linear_to_alaw() logic.
 * The return value is stored in a Uint16, but only low 8 bits are valid.
 */
Uint16 G711A_LinearToAlaw(int16 linear)
{
    Uint16 mask;
    Uint16 alaw;
    int16 seg;
    int32 x;

    x = (int32)linear;

    if (x >= 0L)
    {
        /*
         * Sign bit = 1.
         */
        mask = (Uint16)(G711_ALAW_AMI_MASK | 0x80U);
    }
    else
    {
        /*
         * Sign bit = 0.
         *
         * Use -x - 1 to match the WebRTC/ITU bit-exact behavior.
         * Do the operation in int32 to avoid overflow at -32768.
         */
        mask = G711_ALAW_AMI_MASK;
        x = -x - 1L;
    }

    /*
     * Convert magnitude to segment number.
     *
     * Same idea as:
     *   seg = top_bit(linear | 0xFF) - 7;
     */
    seg = (int16)(G711_TopBit((Uint32)x | 0xFFUL) - 7);

    if (seg >= 8)
    {
        /*
         * Out of range. Return maximum A-law magnitude.
         */
        return (Uint16)((0x7FU ^ mask) & 0x00FFU);
    }

    /*
     * Combine segment and quantization bits.
     */
    if (seg != 0)
    {
        alaw = (Uint16)(((Uint16)seg << 4) |
                        (Uint16)((((Uint32)x) >> ((Uint16)seg + 3U)) & 0x0FUL));
    }
    else
    {
        alaw = (Uint16)(((Uint16)seg << 4) |
                        (Uint16)((((Uint32)x) >> 4U) & 0x0FUL));
    }

    alaw = (Uint16)((alaw ^ mask) & 0x00FFU);

    return alaw;
}

/*
 * G.711 A-law octet -> PCM16 linear sample.
 *
 * Input alaw:
 *   only low 8 bits are used.
 */
int16 G711A_AlawToLinear(Uint16 alaw)
{
    Uint16 a;
    int32 i;
    int16 seg;

    a = (Uint16)(alaw & 0x00FFU);
    a ^= G711_ALAW_AMI_MASK;

    i = (int32)((a & 0x0FU) << 4);
    seg = (int16)((a & 0x70U) >> 4);

    if (seg != 0)
    {
        i = (i + 0x108L) << ((Uint16)seg - 1U);
    }
    else
    {
        i += 8L;
    }

    if ((a & 0x80U) != 0U)
    {
        return (int16)i;
    }
    else
    {
        return (int16)(-i);
    }
}

void G711A_EncodeFrame(const int16 *pcm_in,
                       Uint16 *alaw_out,
                       Uint16 sample_count)
{
    Uint16 i;

    if ((pcm_in == 0) || (alaw_out == 0))
    {
        return;
    }

    for (i = 0U; i < sample_count; i++)
    {
        alaw_out[i] = G711A_LinearToAlaw(pcm_in[i]);
    }
}

void G711A_DecodeFrame(const Uint16 *alaw_in,
                       int16 *pcm_out,
                       Uint16 octet_count)
{
    Uint16 i;

    if ((alaw_in == 0) || (pcm_out == 0))
    {
        return;
    }

    for (i = 0U; i < octet_count; i++)
    {
        pcm_out[i] = G711A_AlawToLinear(alaw_in[i]);
    }
}

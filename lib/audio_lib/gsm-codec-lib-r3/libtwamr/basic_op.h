/*___________________________________________________________________________
 |                                                                           |
 |   Constants and Globals                                                   |
 |___________________________________________________________________________|
*/
extern Flag Overflow;
extern Flag Carry;

#define MAX_32 (Word32)0x7fffffffL
#define MIN_32 (Word32)0x80000000L

#define MAX_16 (Word16)0x7fff
#define MIN_16 (Word16)0x8000

/*___________________________________________________________________________
 |                                                                           |
 |   Prototypes for basic arithmetic operators                               |
 |___________________________________________________________________________|
*/

#if defined(AMR_FAST_BASIC_OPS) && !defined(BASICOP2_IMPLEMENTATION)

static inline Word16 amr_fast_saturate(Word32 L_var1)
{
    if (L_var1 > (Word32)0x00007fffL) {
        Overflow = 1;
        return MAX_16;
    }
    if (L_var1 < (Word32)0xffff8000L) {
        Overflow = 1;
        return MIN_16;
    }
    return (Word16)L_var1;
}

static inline Word16 extract_h(Word32 L_var1) { return (Word16)(L_var1 >> 16); }
static inline Word16 extract_l(Word32 L_var1) { return (Word16)L_var1; }
static inline Word16 add(Word16 var1, Word16 var2) { return amr_fast_saturate((Word32)var1 + var2); }
static inline Word16 sub(Word16 var1, Word16 var2) { return amr_fast_saturate((Word32)var1 - var2); }
static inline Word16 abs_s(Word16 var1) { return (var1 == MIN_16) ? MAX_16 : ((var1 < 0) ? (Word16)-var1 : var1); }
static inline Word16 negate(Word16 var1) { return (var1 == MIN_16) ? MAX_16 : (Word16)-var1; }
static inline Word16 shr(Word16 var1, Word16 var2);

static inline Word16 shl(Word16 var1, Word16 var2)
{
    Word32 result;
    if (var2 < 0) {
        if (var2 < -16) var2 = -16;
        return shr(var1, (Word16)-var2);
    }
    result = (Word32)var1 * ((Word32)1 << var2);
    if ((var2 > 15 && var1 != 0) || (result != (Word32)((Word16)result))) {
        Overflow = 1;
        return (var1 > 0) ? MAX_16 : MIN_16;
    }
    return (Word16)result;
}

static inline Word16 shr(Word16 var1, Word16 var2)
{
    if (var2 < 0) {
        if (var2 < -16) var2 = -16;
        return shl(var1, (Word16)-var2);
    }
    if (var2 >= 15) return (var1 < 0) ? (Word16)-1 : (Word16)0;
    if (var1 < 0) return (Word16)(~((~var1) >> var2));
    return (Word16)(var1 >> var2);
}

static inline Word16 mult(Word16 var1, Word16 var2)
{
    Word32 L_product = (Word32)var1 * (Word32)var2;
    L_product = (L_product & (Word32)0xffff8000L) >> 15;
    if (L_product & (Word32)0x00010000L) L_product |= (Word32)0xffff0000L;
    return amr_fast_saturate(L_product);
}

static inline Word32 L_mult(Word16 var1, Word16 var2)
{
    Word32 L_var_out = (Word32)var1 * (Word32)var2;
    if (L_var_out != (Word32)0x40000000L) {
        L_var_out *= 2;
    } else {
        Overflow = 1;
        L_var_out = MAX_32;
    }
    return L_var_out;
}

static inline Word32 L_add(Word32 L_var1, Word32 L_var2)
{
    Word32 L_var_out = L_var1 + L_var2;
    if (((L_var1 ^ L_var2) & MIN_32) == 0) {
        if ((L_var_out ^ L_var1) & MIN_32) {
            L_var_out = (L_var1 < 0) ? MIN_32 : MAX_32;
            Overflow = 1;
        }
    }
    return L_var_out;
}

static inline Word32 L_sub(Word32 L_var1, Word32 L_var2)
{
    Word32 L_var_out = L_var1 - L_var2;
    if (((L_var1 ^ L_var2) & MIN_32) != 0) {
        if ((L_var_out ^ L_var1) & MIN_32) {
            L_var_out = (L_var1 < 0L) ? MIN_32 : MAX_32;
            Overflow = 1;
        }
    }
    return L_var_out;
}

static inline Word16 round(Word32 L_var1) { return extract_h(L_add(L_var1, (Word32)0x00008000L)); }
static inline Word32 L_mac(Word32 L_var3, Word16 var1, Word16 var2) { return L_add(L_var3, L_mult(var1, var2)); }
static inline Word32 L_msu(Word32 L_var3, Word16 var1, Word16 var2) { return L_sub(L_var3, L_mult(var1, var2)); }

static inline Word16 mult_r(Word16 var1, Word16 var2)
{
    Word32 L_product_arr = (Word32)var1 * (Word32)var2;
    L_product_arr += (Word32)0x00004000L;
    L_product_arr &= (Word32)0xffff8000L;
    L_product_arr >>= 15;
    if (L_product_arr & (Word32)0x00010000L) L_product_arr |= (Word32)0xffff0000L;
    return amr_fast_saturate(L_product_arr);
}

static inline Word32 L_shr(Word32 L_var1, Word16 var2);

static inline Word32 L_shl(Word32 L_var1, Word16 var2)
{
    Word32 L_var_out = L_var1;
    if (var2 <= 0) {
        if (var2 < -32) var2 = -32;
        return L_shr(L_var1, (Word16)-var2);
    }
    for (; var2 > 0; var2--) {
        if (L_var_out > (Word32)0x3fffffffL) {
            Overflow = 1;
            return MAX_32;
        }
        if (L_var_out < (Word32)0xc0000000L) {
            Overflow = 1;
            return MIN_32;
        }
        L_var_out *= 2;
    }
    return L_var_out;
}

static inline Word32 L_shr(Word32 L_var1, Word16 var2)
{
    if (var2 < 0) {
        if (var2 < -32) var2 = -32;
        return L_shl(L_var1, (Word16)-var2);
    }
    if (var2 >= 31) return (L_var1 < 0L) ? (Word32)-1 : (Word32)0;
    if (L_var1 < 0) return ~((~L_var1) >> var2);
    return L_var1 >> var2;
}

static inline Word16 shr_r(Word16 var1, Word16 var2)
{
    Word16 var_out;
    if (var2 > 15) return 0;
    var_out = shr(var1, var2);
    if ((var2 > 0) && ((var1 & ((Word16)1 << (var2 - 1))) != 0)) var_out++;
    return var_out;
}

static inline Word16 mac_r(Word32 L_var3, Word16 var1, Word16 var2) { return extract_h(L_add(L_mac(L_var3, var1, var2), (Word32)0x00008000L)); }
static inline Word16 msu_r(Word32 L_var3, Word16 var1, Word16 var2) { return extract_h(L_add(L_msu(L_var3, var1, var2), (Word32)0x00008000L)); }
static inline Word32 L_deposit_h(Word16 var1) { return (Word32)var1 << 16; }
static inline Word32 L_deposit_l(Word16 var1) { return (Word32)var1; }

static inline Word32 L_shr_r(Word32 L_var1, Word16 var2)
{
    Word32 L_var_out;
    if (var2 > 31) return 0;
    L_var_out = L_shr(L_var1, var2);
    if ((var2 > 0) && ((L_var1 & ((Word32)1 << (var2 - 1))) != 0)) L_var_out++;
    return L_var_out;
}

static inline Word32 L_abs(Word32 L_var1) { return (L_var1 == MIN_32) ? MAX_32 : ((L_var1 < 0) ? -L_var1 : L_var1); }

static inline Word32 L_sat(Word32 L_var1)
{
    Word32 L_var_out = L_var1;
    if (Overflow) {
        L_var_out = Carry ? MIN_32 : MAX_32;
        Carry = 0;
        Overflow = 0;
    }
    return L_var_out;
}

static inline Word16 norm_s(Word16 var1)
{
    Word16 var_out;
    if (var1 == 0) return 0;
    if (var1 == (Word16)0xffff) return 15;
    if (var1 < 0) var1 = ~var1;
    for (var_out = 0; var1 < 0x4000; var_out++) var1 <<= 1;
    return var_out;
}

static inline Word16 norm_l(Word32 L_var1)
{
    Word16 var_out;
    if (L_var1 == 0) return 0;
    if (L_var1 == (Word32)0xffffffffL) return 31;
    if (L_var1 < 0) L_var1 = ~L_var1;
    for (var_out = 0; L_var1 < (Word32)0x40000000L; var_out++) L_var1 <<= 1;
    return var_out;
}

Word32 L_macNs (Word32 L_var3, Word16 var1, Word16 var2);
Word32 L_msuNs (Word32 L_var3, Word16 var1, Word16 var2);
Word32 L_add_c (Word32 L_var1, Word32 L_var2);
Word32 L_sub_c (Word32 L_var1, Word32 L_var2);
Word32 L_negate (Word32 L_var1);
Word16 div_s (Word16 var1, Word16 var2);

#else

Word16 add (Word16 var1, Word16 var2);    /* Short add,           1   */
Word16 sub (Word16 var1, Word16 var2);    /* Short sub,           1   */
Word16 abs_s (Word16 var1);               /* Short abs,           1   */
Word16 shl (Word16 var1, Word16 var2);    /* Short shift left,    1   */
Word16 shr (Word16 var1, Word16 var2);    /* Short shift right,   1   */
Word16 mult (Word16 var1, Word16 var2);   /* Short mult,          1   */
Word32 L_mult (Word16 var1, Word16 var2); /* Long mult,           1   */
Word16 negate (Word16 var1);              /* Short negate,        1   */
Word16 extract_h (Word32 L_var1);         /* Extract high,        1   */
Word16 extract_l (Word32 L_var1);         /* Extract low,         1   */
Word16 round (Word32 L_var1);             /* Round,               1   */
Word32 L_mac (Word32 L_var3, Word16 var1, Word16 var2);   /* Mac,  1  */
Word32 L_msu (Word32 L_var3, Word16 var1, Word16 var2);   /* Msu,  1  */
Word32 L_macNs (Word32 L_var3, Word16 var1, Word16 var2); /* Mac without
                                                             sat, 1   */
Word32 L_msuNs (Word32 L_var3, Word16 var1, Word16 var2); /* Msu without
                                                             sat, 1   */
Word32 L_add (Word32 L_var1, Word32 L_var2);    /* Long add,        2 */
Word32 L_sub (Word32 L_var1, Word32 L_var2);    /* Long sub,        2 */
Word32 L_add_c (Word32 L_var1, Word32 L_var2);  /* Long add with c, 2 */
Word32 L_sub_c (Word32 L_var1, Word32 L_var2);  /* Long sub with c, 2 */
Word32 L_negate (Word32 L_var1);                /* Long negate,     2 */
Word16 mult_r (Word16 var1, Word16 var2);       /* Mult with round, 2 */
Word32 L_shl (Word32 L_var1, Word16 var2);      /* Long shift left, 2 */
Word32 L_shr (Word32 L_var1, Word16 var2);      /* Long shift right, 2*/
Word16 shr_r (Word16 var1, Word16 var2);        /* Shift right with
                                                   round, 2           */
Word16 mac_r (Word32 L_var3, Word16 var1, Word16 var2); /* Mac with
                                                           rounding,2 */
Word16 msu_r (Word32 L_var3, Word16 var1, Word16 var2); /* Msu with
                                                           rounding,2 */
Word32 L_deposit_h (Word16 var1);        /* 16 bit var1 -> MSB,     2 */
Word32 L_deposit_l (Word16 var1);        /* 16 bit var1 -> LSB,     2 */

Word32 L_shr_r (Word32 L_var1, Word16 var2); /* Long shift right with
                                                round,  3             */
Word32 L_abs (Word32 L_var1);            /* Long abs,              3  */
Word32 L_sat (Word32 L_var1);            /* Long saturation,       4  */
Word16 norm_s (Word16 var1);             /* Short norm,           15  */
Word16 div_s (Word16 var1, Word16 var2); /* Short division,       18  */
Word16 norm_l (Word32 L_var1);           /* Long norm,            30  */   

#endif

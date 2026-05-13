/*
 * Unfortunately the code we got from ETSI makes heavy use of two global
 * Boolean flags named Carry and Overflow that function like equally named
 * processor state flags on many CPU architectures.  They are not part
 * of persistent codec session state for either the encoder or the decoder,
 * instead they are "short-term" globals much like UNIX errno.
 *
 * Given this unfortunate reality plus the natural desire to make our
 * AMR library thread-safe (a transcoding MGW handling a large volume of
 * simultaneous calls is exactly the kind of application that would benefit
 * from utilitizing all CPU cores), our current workaround is to use
 * thread-local storage.
 */

#include "typedef.h"
#include "namespace.h"

//__thread Flag Carry, Overflow;

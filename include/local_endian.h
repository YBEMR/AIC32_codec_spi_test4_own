/*
 * This header file defines the interface to our utility function
 * that determines the local machine's native byte order, thereby
 * providing a default for file reading functions that can be overridden
 * with -b or -l options.
 */

int is_native_big_endian(void);

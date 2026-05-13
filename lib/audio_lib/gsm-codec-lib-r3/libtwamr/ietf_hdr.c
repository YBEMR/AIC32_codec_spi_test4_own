/*
 * This module provides one public const datum: RFC 4867 file header.
 */

#include <stdint.h>
#include "tw_amr.h"

const uint8_t amr_file_header_magic[AMR_IETF_HDR_LEN] = {'#', '!', 'A', 'M', 'R', '\n',};

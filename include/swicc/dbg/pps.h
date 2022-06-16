#pragma once

#include "swicc/common.h"
#include "swicc/pps.h"

swicc_ret_et swicc_dbg_pps_str(char *const buf_str, uint16_t *const buf_str_len,
                               uint8_t const *const buf_pps,
                               uint16_t const buf_pps_len);

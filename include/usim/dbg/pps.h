#include "usim/common.h"
#include "usim/pps.h"

#pragma once

usim_ret_et usim_dbg_str_pps(char *const buf_str, uint16_t *const buf_str_len,
                             uint8_t const *const buf_pps,
                             uint16_t const buf_pps_len);

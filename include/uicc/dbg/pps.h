#include "uicc/common.h"
#include "uicc/pps.h"

#pragma once

uicc_ret_et uicc_dbg_pps_str(char *const buf_str, uint16_t *const buf_str_len,
                             uint8_t const *const buf_pps,
                             uint16_t const buf_pps_len);

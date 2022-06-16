#pragma once

#include "swicc/common.h"

swicc_ret_et swicc_dbg_atr_str(char *const buf_str, uint16_t *const buf_str_len,
                               uint8_t const *const buf_atr,
                               uint16_t const buf_atr_len);

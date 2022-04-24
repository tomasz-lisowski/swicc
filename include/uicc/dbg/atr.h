#include "uicc/common.h"

#pragma once

uicc_ret_et uicc_dbg_atr_str(char *const buf_str, uint16_t *const buf_str_len,
                             uint8_t const *const buf_atr,
                             uint16_t const buf_atr_len);

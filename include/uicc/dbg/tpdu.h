#include "uicc/common.h"
#include "uicc/tpdu.h"

#pragma once

uicc_ret_et uicc_dbg_tpdu_cmd_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_tpdu_cmd_st const *const tpdu_cmd);

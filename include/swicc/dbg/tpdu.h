#pragma once

#include "swicc/common.h"
#include "swicc/tpdu.h"

swicc_ret_et swicc_dbg_tpdu_cmd_str(char *const buf_str,
                                    uint16_t *const buf_str_len,
                                    swicc_tpdu_cmd_st const *const tpdu_cmd);

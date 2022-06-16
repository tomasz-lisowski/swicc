#pragma once

#include "swicc/common.h"
#include "swicc/fsm.h"

swicc_ret_et swicc_dbg_fsm_state_str(char *const buf_str,
                                     uint16_t *const buf_str_len,
                                     swicc_fsm_state_et const fsm_state);

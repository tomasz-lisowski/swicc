#include "uicc/common.h"
#include "uicc/fsm.h"

#pragma once

uicc_ret_et uicc_dbg_fsm_state_str(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   uicc_fsm_state_et fsm_state);

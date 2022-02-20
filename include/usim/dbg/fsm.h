#include "usim/common.h"
#include "usim/fsm.h"

#pragma once

usim_ret_et usim_dbg_str_fsm_state(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   usim_fsm_state_et fsm_state);

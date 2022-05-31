#pragma once

#include "uicc/common.h"

uicc_ret_et uicc_dbg_io_cont_str(char *const buf_str,
                                 uint16_t *const buf_str_len,
                                 uint32_t const cont_state_rx,
                                 uint32_t const cont_state_tx);

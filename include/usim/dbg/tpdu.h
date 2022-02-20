#include "usim/common.h"
#include "usim/tpdu.h"

#pragma once

usim_ret_et usim_str_tpdu_cmd(char *const buf_str, uint16_t *const buf_str_len,
                              tpdu_cmd_st const *const tpdu_cmd);

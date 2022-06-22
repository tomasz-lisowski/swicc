#pragma once

#include "swicc/common.h"

swicc_ret_et swicc_dbg_net_msg_str(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   char const *const prestr,
                                   swicc_net_msg_st const *const msg);

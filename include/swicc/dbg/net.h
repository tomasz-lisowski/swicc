#pragma once

#include "swicc/common.h"

/**
 * @brief Get a string for a network message.
 * @param[out] buf_str Where to write the string.
 * @param[in, out] buf_str_len Must contain the max string length. Receives
 * length of written string on success.
 * @param[in] prestr A string to prepend to the created string. E.g. "TX" or
 * "RX" to differentiate direction.
 * @param[in] msg
 * @return Return code.
 */
swicc_ret_et swicc_dbg_net_msg_str(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   char const *const prestr,
                                   swicc_net_msg_st const *const msg);

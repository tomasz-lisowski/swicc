#pragma once

#include "swicc/common.h"

/**
 * @brief Get a string that represents the contact states.
 * @param[out] buf_str Where to write the string.
 * @param[in, out] buf_str_len Must contain the max string length. Receives
 * length of written string on success.
 * @param[in] cont_state_rx
 * @param[in] cont_state_tx
 * @return Return code.
 */
swicc_ret_et swicc_dbg_io_cont_str(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   uint32_t const cont_state_rx,
                                   uint32_t const cont_state_tx);

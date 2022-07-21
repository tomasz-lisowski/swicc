#pragma once

#include "swicc/common.h"
#include "swicc/pps.h"

/**
 * @brief Generate a string for a PPS message to make all the confi and protocol
 * params human-readable.
 * @param[out] buf_str Where to write the string.
 * @param[in, out] buf_str_len Must contain the max string length. Receives
 * length of written string on success.
 * @param[in] buf_pps The PPS message.
 * @param[in] buf_pps_len Length of the PPS message.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_pps_str(char *const buf_str, uint16_t *const buf_str_len,
                               uint8_t const *const buf_pps,
                               uint16_t const buf_pps_len);

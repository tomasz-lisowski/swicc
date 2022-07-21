#pragma once

#include "swicc/common.h"

/**
 * @brief Parse an ATR and convert it into a human-readable string.
 * @param[out] buf_str
 * @param[in, out] buf_str_len Maximum length of the string. Gets the length of
 * the written string on success.
 * @param[in] buf_atr Buffer containing the ATR.
 * @param[in] buf_atr_len Length of the ATR.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_atr_str(char *const buf_str, uint16_t *const buf_str_len,
                               uint8_t const *const buf_atr,
                               uint16_t const buf_atr_len);

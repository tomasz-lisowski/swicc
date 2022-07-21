#pragma once

#include "swicc/common.h"
#include "swicc/tpdu.h"

/**
 * @brief Generate a string of a TPDU command.
 * @param[out] buf_str Where to write the string.
 * @param[in, out] buf_str_len Must contain max string length. Receives length
 * of written string on success.
 * @param[in] tpdu_cmd The TPDU to stringify.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_tpdu_cmd_str(char *const buf_str,
                                    uint16_t *const buf_str_len,
                                    swicc_tpdu_cmd_st const *const tpdu_cmd);

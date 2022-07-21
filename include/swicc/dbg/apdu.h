#pragma once

#include "swicc/apdu.h"
#include "swicc/common.h"

/**
 * @brief Generate a string representation of a C-APDU.
 * @param[out] buf_str Where to write the string.
 * @param[in, out] buf_str_len Maximum length of the string. This gets modified
 * to contain the actual length of string written to the buffer.
 * @param[in] apdu_cmd The APDU that will be converted to a string.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_apdu_cmd_str(char *const buf_str,
                                    uint16_t *const buf_str_len,
                                    swicc_apdu_cmd_st const *const apdu_cmd);

/**
 * @brief Generate a string representation of an R-APDU.
 * @param[out] buf_str Where to write the string.
 * @param[in, out] buf_str_len Maximum length of the string. This gets modified
 * to contain the actual length of string written to the buffer.
 * @param[in] apdu_res The response that will be converted to a string.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_apdu_res_str(char *const buf_str,
                                    uint16_t *const buf_str_len,
                                    swicc_apdu_res_st const *const apdu_res);

/**
 * @brief Check the command chaining control info and return the string that
 * describes it.
 * @param[in] cla
 * @return Pointer to a constant CCC string, do not free, just forget it.
 */
char const *swicc_dbg_apdu_cla_ccc_str(swicc_apdu_cla_st const cla);

/**
 * @brief Check the secure messaging indication and return the string that
 * describes it.
 * @param[in] cla
 * @return Pointer to a constant SM string, do not free, just forget it.
 */
char const *swicc_dbg_apdu_cla_sm_str(swicc_apdu_cla_st const cla);

/**
 * @brief Check the type of the CLA and return the string that describes it.
 * @param[in] cla
 * @return Pointer to a constant type string, do not free, just forget it.
 */
char const *swicc_dbg_apdu_cla_type_str(swicc_apdu_cla_st const cla);

/**
 * @brief Map an instruction byte to the instruction string.
 * @param[in] ins
 * @return Pointer to a constant type string, do not free, just forget it.
 */
char const *swicc_dbg_apdu_ins_str(uint8_t const ins);

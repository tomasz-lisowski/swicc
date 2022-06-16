#pragma once

#include "swicc/apdu.h"
#include "swicc/common.h"

/**
 * @brief Generate a string which describes all fields of the parsed APDU
 * command.
 * @param buf_str Where the string will be written.
 * @param buf_str_len Maximum length of the string. This gets modified to
 * contain the actual length of string written to the buffer.
 * @param apdu_cmd The APDU that will be converted to a string.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_apdu_cmd_str(char *const buf_str,
                                    uint16_t *const buf_str_len,
                                    swicc_apdu_cmd_st const *const apdu_cmd);

swicc_ret_et swicc_dbg_apdu_res_str(char *const buf_str,
                                    uint16_t *const buf_str_len,
                                    swicc_apdu_res_st const *const apdu_res);

char const *swicc_dbg_apdu_cla_ccc_str(swicc_apdu_cla_st const cla);
char const *swicc_dbg_apdu_cla_sm_str(swicc_apdu_cla_st const cla);
char const *swicc_dbg_apdu_cla_type_str(swicc_apdu_cla_st const cla);
char const *swicc_dbg_apdu_ins_str(uint8_t const ins);

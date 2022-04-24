#include "uicc/apdu.h"
#include "uicc/common.h"

#pragma once

/**
 * @brief Generate a string which describes all fields of the parsed APDU
 * command.
 * @param buf_str Where the string will be written.
 * @param buf_str_len Maximum length of the string. This gets modified to
 * contain the actual length of string written to the buffer.
 * @param apdu_cmd The APDU that will be converted to a string.
 * @return Return code.
 */
uicc_ret_et uicc_dbg_apdu_cmd_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_apdu_cmd_st const *const apdu_cmd);

uicc_ret_et uicc_dbg_apdu_res_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_apdu_res_st const *const apdu_res);

char const *uicc_dbg_apdu_cla_ccc_str(uicc_apdu_cla_st const cla);
char const *uicc_dbg_apdu_cla_sm_str(uicc_apdu_cla_st const cla);
char const *uicc_dbg_apdu_cla_type_str(uicc_apdu_cla_st const cla);
char const *uicc_dbg_apdu_ins_str(uint8_t const ins);

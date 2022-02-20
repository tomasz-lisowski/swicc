#include "usim/apdu.h"
#include "usim/common.h"

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
usim_ret_et usim_dbg_str_apdu_cmd(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  apdu_cmd_st const *const apdu_cmd);

usim_ret_et usim_dbg_str_apdu_res(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  apdu_res_st const *const apdu_res);

char const *usim_dbg_str_apdu_cla_chain(apdu_cla_et const cla);
char const *usim_dbg_str_apdu_cla_sm(apdu_cla_et const cla);
char const *usim_dbg_str_apdu_cla_info(apdu_cla_et const cla);
uint8_t usim_dbg_str_apdu_cla_lchan(apdu_cla_et const cla);
char const *usim_dbg_str_apdu_ins(apdu_ins_et const ins);

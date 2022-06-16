#include "swicc/apdu.h"
#include "swicc/common.h"

#pragma once
/**
 * TPDU = Transmission protocol data unit
 */

/**
 * An internal format of the TPDU command which is the result of parsing a raw
 * TPDU command.
 */
typedef struct swicc_tpdu_cmd_s
{
    swicc_apdu_cmd_hdr_st hdr;
    uint8_t p3;
    swicc_apdu_data_st data;
} swicc_tpdu_cmd_st;

swicc_ret_et swicc_tpdu_cmd_parse(uint8_t const *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  swicc_tpdu_cmd_st *const cmd);

/**
 * @brief Extract the APDU contained in the TPDU.
 * @param apdu_cmd
 * @param tpdu_cmd
 * @return Return code.
 * @note The APDU contains references to the TPDU data to avoid copying so the
 * TPDU must remain valid for as long as the APDU is needed.
 */
swicc_ret_et swicc_tpdu_to_apdu(swicc_apdu_cmd_st *const apdu_cmd,
                                swicc_tpdu_cmd_st *const tpdu_cmd);

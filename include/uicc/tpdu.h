#include "uicc/apdu.h"
#include "uicc/common.h"

#pragma once
/**
 * TPDU = Transmission protocol data unit
 */

/**
 * An internal format of the TPDU command which is the result of parsing a raw
 * TPDU command.
 */
typedef struct uicc_tpdu_cmd_s
{
    uicc_apdu_cmd_hdr_st hdr;
    uint8_t p3;
    uicc_apdu_data_st data;
} uicc_tpdu_cmd_st;

uicc_ret_et uicc_tpdu_cmd_parse(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                uicc_tpdu_cmd_st *const cmd);

/**
 * @brief Extract the APDU contained in the TPDU.
 * @param apdu_cmd
 * @param tpdu_cmd
 * @return Return code.
 * @note The APDU contains references to the TPDU data to avoid copying so the
 * TPDU must remain valid for as long as the APDU is needed.
 */
uicc_ret_et uicc_tpdu_to_apdu(uicc_apdu_cmd_st *const apdu_cmd,
                              uicc_tpdu_cmd_st *const tpdu_cmd);

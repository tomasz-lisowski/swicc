#include "uicc/apdu.h"
#include "uicc/common.h"

#pragma once
/**
 * TPDU = Transport protocol data unit
 */

/**
 * The header as it comes on the wire.
 */
typedef struct uicc_tpdu_cmd_hdr_raw_s
{
    uicc_apdu_cmd_hdr_raw_st hdr_apdu;
    uint8_t p3;
} __attribute__((__packed__)) uicc_tpdu_cmd_hdr_raw_st;

/**
 * An internal format of the TPDU command header.
 */
typedef struct uicc_tpdu_cmd_hdr_s
{
    uicc_apdu_cmd_hdr_st hdr_apdu;
    uint8_t param_3;
} uicc_tpdu_cmd_hdr_st;

/**
 * An internal format of the TPDU command which is the result of parsing a raw
 * TPDU command.
 */
typedef struct uicc_tpdu_cmd_s
{
    uicc_tpdu_cmd_hdr_st hdr;
    uint16_t data_len_cmd;
    uint16_t data_len_res;
    uint8_t data[UICC_DATA_MAX];
} uicc_tpdu_cmd_st;

uicc_ret_et uicc_tpdu_cmd_parse(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                uicc_tpdu_cmd_st *const tpdu_cmd);

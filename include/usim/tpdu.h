#include <stdint.h>
#include <usim/apdu.h>
#include <usim/common.h>

#pragma once
/**
 * TPDU = Transport protocol data unit
 */

/**
 * The header as it comes on the wire.
 */
typedef struct tpdu_cmd_hdr_raw_s
{
    apdu_cmd_hdr_raw_st hdr_apdu;
    uint8_t p3;
} __attribute__((__packed__)) tpdu_cmd_hdr_raw_st;

/**
 * An internal format of the TPDU command header.
 */
typedef struct tpdu_cmd_hdr_s
{
    apdu_cmd_hdr_st hdr_apdu;
    uint8_t param_3;
} tpdu_cmd_hdr_st;

/**
 * An internal format of the TPDU command which is the result of parsing a raw
 * TPDU command.
 */
typedef struct tpdu_cmd_s
{
    tpdu_cmd_hdr_st hdr;
    uint16_t data_len_cmd;
    uint16_t data_len_res;
    uint8_t data[USIM_DATA_MAX];
} tpdu_cmd_st;

usim_ret_et usim_parse_tpdu_cmd(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                tpdu_cmd_st *const tpdu_cmd);

#include <assert.h>
#include <stdint.h>
#include <usim/common.h>

#pragma once
/**
 * APDU = Application protocol data unit
 */

/**
 * Collection of flags describing all aspects of an APDU command class.
 */
typedef enum apdu_cla_e
{
    /* Command chaining control */
    APDU_CLA_CMD_CHAIN_LAST = 1U,
    APDU_CLA_CMD_CHAIN_MORE = 1U << 1U,

    /* Secure Messaging (SM) indication */
    APDU_CLA_SM_NO = 1U << 2U,           /* NO SM or no indication */
    APDU_CLA_SM_PROPRIETARY = 1U << 3U,  /* Properietary SM format */
    APDU_CLA_SM_CMD_HDR_SKIP = 1U << 4U, /* Std SM and cmd header unsecure */
    APDU_CLA_SM_CMD_HDR_AUTH = 1U << 5U, /* Std SM and cmd header secure */

    APDU_CLA_INTERINDUSTRY = 1U << 23U,
    APDU_CLA_PROPRIETARY = 1U << 24U,
    APDU_CLA_RFU = 1U << 25U, /* Reserved for future use */
    APDU_CLA_INVALID = 1U << 26U,

    /* Logical channel number (CLA >> 27) */
    APDU_CLA_LCHAN_B0 = 1U << 27U,
    APDU_CLA_LCHAN_B1 = 1U << 28U,
    APDU_CLA_LCHAN_B2 = 1U << 29U,
    APDU_CLA_LCHAN_B3 = 1U << 30U,
} apdu_cla_et;
static_assert(sizeof(apdu_cla_et) == 4, "CLA enum type must be 4 bytes wide");

/**
 * A self documentingm member definition for every APDU command instruction.
 * ISO 7816-4:2020 p.15-16
 */
typedef enum apdu_ins_e
{
    APDU_INS_FILE_DEACTIVATE = 0x04,
    APDU_INS_FILE_ACTIVATE = 0x44,
    APDU_INS_FILE_CREATE = 0xE0,
    APDU_INS_FILE_DELETE = 0xE4,
    APDU_INS_FILE_DF_TERMINATE = 0xE6,
    APDU_INS_FILE_EF_TERMINATE = 0xE8,

    APDU_INS_RECORD_DEACTIVATE = 0x06,
    APDU_INS_RECORD_ACTIVATE = 0x08,
    APDU_INS_RECORD_ERASE = 0x0C,
    APDU_INS_RECORD_SEARCH = 0xA2,
    APDU_INS_RECORD_READ0 = 0xB2,
    APDU_INS_RECORD_READ1 = 0xB3,
    APDU_INS_RECORD_WRITE = 0xD2,
    APDU_INS_RECORD_UPDATE0 = 0xDC,
    APDU_INS_RECORD_UPDATE1 = 0xDD,
    APDU_INS_RECORD_APPEND = 0xE2,

    APDU_INS_BIN_ERASE0 = 0x0E,
    APDU_INS_BIN_ERASE1 = 0x0F,
    APDU_INS_BIN_SEARCH0 = 0xA0,
    APDU_INS_BIN_SEARCH1 = 0xA1,
    APDU_INS_BIN_READ0 = 0xB0,
    APDU_INS_BIN_READ1 = 0xB1,
    APDU_INS_BIN_WRITE0 = 0xD0,
    APDU_INS_BIN_WRITE1 = 0xD1,
    APDU_INS_BIN_UPDATE0 = 0xD6,
    APDU_INS_BIN_UPDATE1 = 0xD7,

    APDU_INS_OP_SCQL = 0x10,
    APDU_INS_OP_TRANSACTION = 0x12,
    APDU_INS_OP_USER = 0x14,
    APDU_INS_OP_SECURITY0 = 0x2A,
    APDU_INS_OP_SECURITY1 = 0x2B,
    APDU_INS_OP_BIOMETRIC0 = 0x2E,
    APDU_INS_OP_BIOMETRIC1 = 0x2F,

    APDU_INS_CRYPTO_VERIFY0 = 0x20,
    APDU_INS_CRYPTO_VERIFY1 = 0x21,
    APDU_INS_CRYPTO_SECENV_MANAGE = 0x22,
    APDU_INS_CRYPTO_ASYM_KEY_PAIR_GEN0 = 0x46,
    APDU_INS_CRYPTO_ASYM_KEY_PAIR_GEN1 = 0x47,
    APDU_INS_CRYPTO_CARD_SECRET_IMPORT = 0x48,
    APDU_INS_CRYPTO_AUTH_EXTERNAL = 0x82,
    APDU_INS_CRYPTO_CHALLENGE_GET = 0x84,
    APDU_INS_CRYPTO_AUTH_GENERAL0 = 0x86,
    APDU_INS_CRYPTO_AUTH_GENERAL1 = 0x87,
    APDU_INS_CRYPTO_AUTH_INTERNAL = 0x88,
    APDU_INS_CRYPTO_VERIF_REQ_DISABLE = 0x26,
    APDU_INS_CRYPTO_VERIF_REQ_ENABLE = 0x28,

    APDU_INS_APP_MGMT_REQ0 = 0x40,
    APDU_INS_APP_MGMT_REQ1 = 0x41,
    APDU_INS_APP_LOAD0 = 0xEA,
    APDU_INS_APP_LOAD1 = 0xEB,
    APDU_INS_APP_REMOVE0 = 0xEC,
    APDU_INS_APP_REMOVE1 = 0xED,

    APDU_INS_DATA_SELECT = 0xA5,
    APDU_INS_DATA_GET0 = 0xCA,
    APDU_INS_DATA_GET1 = 0xCB,
    APDU_INS_DATA_GET2 = 0xCC,
    APDU_INS_DATA_GET3 = 0xCD,
    APDU_INS_DATA_MANAGE = 0xCF,
    APDU_INS_DATA_NEXT_PUT0 = 0xD8,
    APDU_INS_DATA_NEXT_PUT1 = 0xD9,
    APDU_INS_DATA_PUT0 = 0xDA,
    APDU_INS_DATA_PUT1 = 0xDB,
    APDU_INS_DATA_UPDATE0 = 0xDE,
    APDU_INS_DATA_UPDATE1 = 0xDF,
    APDU_INS_DATA_DELETE = 0xEE,

    APDU_INS_DEVMGMT0 = 0x16,
    APDU_INS_DEVMGMT1 = 0x17,
    APDU_INS_REFDATA_CHANGE0 = 0x24,
    APDU_INS_REFDATA_CHANGE1 = 0x25,
    APDU_INS_RETRYCTR_RESET0 = 0x2C,
    APDU_INS_RETRYCTR_RESET1 = 0x2D,
    APDU_INS_COMPARE = 0x33,
    APDU_INS_ATTRIB_GET0 = 0x34,
    APDU_INS_ATTRIB_GET1 = 0x35,
    APDU_INS_CHAN_MANAGE = 0x70,
    APDU_INS_SELECT = 0xA4,
    APDU_INS_RES_GET = 0xC0,
    APDU_INS_ENVELOPE0 = 0xC2,
    APDU_INS_ENVELOPE1 = 0xC3,
    APDU_INS_CREATE = 0xE1,
    APDU_INS_CARD_TERMINATE = 0xFE,

    APDU_INS_RFU /* Anything no used is RFU (no INS is invalid) */,
} apdu_ins_et;

/**
 * For describing the status word in an APDU response.
 */
typedef enum apdu_sw_e
{
    APDU_SW_SUCCESS,
} apdu_status_et;

/**
 * For indicating the format of the data field of an APDU message (command or
 * response).
 */
typedef enum apdu_data_frmt_e
{
    APDU_DATA_FRMT_NO = 0,          /* No data field format indication */
    APDU_DATA_FRMT_BERTLV = 1 << 0, /* Data field format encoded in BER-TLV */
    /* BER-TLV in ISO7816-4:2020 sec.8.1 */
} apdu_data_frmt_et;

/**
 * The header as it comes on the wire.
 */
typedef struct apdu_cmd_hdr_raw_s
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
} __attribute__((__packed__)) apdu_cmd_hdr_raw_st;

/**
 * An internal format of the header that is easier and more efficient to use in
 * code.
 */
typedef struct apdu_cmd_hdr_s
{
    apdu_cla_et class;
    apdu_ins_et instruction;
    apdu_data_frmt_et data_frmt;
    uint8_t param_1;
    uint8_t param_2;
} apdu_cmd_hdr_st;

/**
 * An internal format of the APDU command which is the result of parsing a raw
 * APDU command.
 */
typedef struct apdu_cmd_s
{
    apdu_cmd_hdr_st hdr;
    uint16_t data_len_cmd;
    uint16_t data_len_res;
    uint8_t data[USIM_DATA_MAX];
} apdu_cmd_st;

/**
 * An internal format of the APDU response which is the result of parsing a raw
 * APDU response.
 */
typedef struct apdu_res_s
{
    uint16_t status;
    uint8_t data[USIM_DATA_MAX];
} apdu_res_st;

/**
 * @brief Parse the raw CLA byte to an OR'd collection of CLA flags.
 * @param cla_raw The class byte of an APDU message.
 * @return Value consisting of OR'd CLA flags.
 */
apdu_cla_et usim_parse_apdu_cmd_cla(uint8_t const cla_raw);

/**
 * @brief Parse the raw INS byte to a valid enum member (otherwise RFU).
 * @param ins_raw The instruction byte of an APDU message.
 * @return Member of INS enum type.
 */
apdu_ins_et usim_parse_apdu_cmd_ins(uint8_t const ins_raw);

/**
 * @brief Given a buffer containing a raw APDU message, parse and validate it
 * into a more useful representation.
 * @param buf_raw Buffer containing the raw APDU message.
 * @param buf_raw_len Length of the buffer containing the raw APDU message.
 * @param apdu_cmd Where the parsed APDU will be written.
 * @return Return code.
 */
usim_ret_et usim_parse_apdu_cmd(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                apdu_cmd_st *const apdu_cmd);

usim_ret_et usim_deparse_apdu_cmd(uint8_t *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  apdu_cmd_st const *const apdu_cmd);

usim_ret_et usim_parse_apdu_res(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                apdu_res_st *const apdu_res);

usim_ret_et usim_deparse_apdu_res(uint8_t *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  apdu_res_st const *const apdu_res);

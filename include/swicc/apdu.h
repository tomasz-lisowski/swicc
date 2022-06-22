#pragma once
/**
 * APDU = Application protocol data unit
 */

#include "swicc/apdu_rc.h"
#include "swicc/common.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum swicc_apdu_cla_ccc_e
{
    SWICC_APDU_CLA_CCC_INVALID,
    SWICC_APDU_CLA_CCC_LAST,
    SWICC_APDU_CLA_CCC_MORE,
} swicc_apdu_cla_ccc_et;

typedef enum swicc_apdu_cla_sm_e
{
    SWICC_APDU_CLA_SM_INVALID,      /* Invalid SM indication */
    SWICC_APDU_CLA_SM_NO,           /* NO SM or no indication */
    SWICC_APDU_CLA_SM_PROPRIETARY,  /* Properietary SM format */
    SWICC_APDU_CLA_SM_CMD_HDR_SKIP, /* Std SM and cmd header unsecure */
    SWICC_APDU_CLA_SM_CMD_HDR_AUTH, /* Std SM and cmd header secure */
} swicc_apdu_cla_sm_et;

typedef enum swicc_apdu_cla_type_e
{
    SWICC_APDU_CLA_TYPE_INVALID,
    SWICC_APDU_CLA_TYPE_INTERINDUSTRY,
    SWICC_APDU_CLA_TYPE_PROPRIETARY,
    SWICC_APDU_CLA_TYPE_RFU, /* Reserved for future use */
} swicc_apdu_cla_type_et;

/* First byte of status word. ISO 7816-4:2020 p.17 sec.5.6 table.6. */
typedef enum swicc_apdu_sw1_e
{
    /* Normal processing. */
    SWICC_APDU_SW1_NORM_NONE = 0x90,            /* 9000 */
    SWICC_APDU_SW1_NORM_BYTES_AVAILABLE = 0x61, /* SW2 indicates number of bytes
                                           available. */

    /* Warning processing. */
    SWICC_APDU_SW1_WARN_NVM_CHGN = 0x62, /* No change in NVM. */
    SWICC_APDU_SW1_WARN_NVM_CHGM = 0x63, /* Possible change in NVM. */

    /* Execution error. */
    SWICC_APDU_SW1_EXER_NVM_CHGN = 0x64, /* No change in NVM. */
    SWICC_APDU_SW1_EXER_NVM_CHGM = 0x65, /* Possible change in NVM. */
    SWICC_APDU_SW1_EXER_SEC = 0x66,      /* Security related. */

    /* Checking error. */
    SWICC_APDU_SW1_CHER_LEN = 0x67,      /* Wrong length. */
    SWICC_APDU_SW1_CHER_CLA_FUNC = 0x68, /* Functions in CLA unsupported. */
    SWICC_APDU_SW1_CHER_CMD = 0x69,      /* Command not allowed. */
    SWICC_APDU_SW1_CHER_P1P2_INFO =
        0x6A,                        /* P1 or P2 invalid + more info in SW2. */
    SWICC_APDU_SW1_CHER_P1P2 = 0x6B, /* P1 or P2 invalid. */
    SWICC_APDU_SW1_CHER_LE = 0x6C,   /* Wrong Le field. */
    SWICC_APDU_SW1_CHER_INS = 0x6D,  /* INS not supported or invalid. */
    SWICC_APDU_SW1_CHER_CLA = 0x6E,  /* CLA unsupported. */
    SWICC_APDU_SW1_CHER_UNK = 0x6F,  /* No diagnosis. */

    /* Procedure byte. ISO 7816-3:2006 p.23 sec.10.3.3 table.11. */
    SWICC_APDU_SW1_PROC_NULL = 0x60, /* Request no action on data transfer */

    /**
     * The ACK_ONE and ACK_ALL depend on the instruction so the far end of the
     * range is used in order to avoid collision with real status values.
     * @warning When an ACK is desired, the APDU handler will use these special
     * values instead of setting it to the actual INS or INS ^ 0xFF value unlike
     * what the other values would suggest.
     */
    SWICC_APDU_SW1_PROC_ACK_ONE = 0xFE, /* Acknowledgement leading to transfer
                                   of one more byte of data.  */
    SWICC_APDU_SW1_PROC_ACK_ALL = 0xFF, /* Acknowledgement leading to transfer
                                   of the rest of data.  */
    /**
     * The case where procedure byte is a regular SW1 is handled like the other
     * SW1 values.
     */
} swicc_apdu_sw1_et;

/**
 * Collection of flags describing all aspects of an APDU command class.
 */
typedef struct swicc_apdu_cla_s
{
    uint8_t raw;               /* The raw CLA byte. */
    swicc_apdu_cla_ccc_et ccc; /* Command chaining control (CCC) */
    swicc_apdu_cla_sm_et sm;   /* Secure Messaging (SM) indication */
    swicc_apdu_cla_type_et type;
    uint16_t lchan; /* Logical channel number ((CLA >> 27) & 0x0F) */
} swicc_apdu_cla_st;

/**
 * The header as it comes on the "wire".
 */
typedef struct swicc_apdu_cmd_hdr_raw_s
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
} __attribute__((__packed__)) swicc_apdu_cmd_hdr_raw_st;

/**
 * An internal format of the header that is easier and more efficient to use in
 * code.
 */
typedef struct swicc_apdu_cmd_hdr_s
{
    swicc_apdu_cla_st cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
} swicc_apdu_cmd_hdr_st;

typedef struct swicc_apdu_data_s
{
    uint16_t len;
    uint8_t b[SWICC_DATA_MAX];
} swicc_apdu_data_st;

/**
 * An internal format of the APDU command which is the result of parsing a raw
 * APDU command.
 */
typedef struct swicc_apdu_cmd_s
{
    swicc_apdu_cmd_hdr_st *hdr;
    uint8_t *p3;
    swicc_apdu_data_st *data;
} swicc_apdu_cmd_st;

/**
 * An internal format of the APDU response which is the result of parsing a raw
 * APDU response.
 */
typedef struct swicc_apdu_res_s
{
    swicc_apdu_sw1_et sw1;
    uint8_t sw2;
    swicc_apdu_data_st data;
} swicc_apdu_res_st;

/**
 * @brief Parse the raw CLA byte.
 * @param cla_raw The class byte of an APDU message.
 * @return Parsed CLA.
 */
swicc_apdu_cla_st swicc_apdu_cmd_cla_parse(uint8_t const cla_raw);

/**
 * @brief Given a buffer containing a raw interindustry APDU message, parse and
 * validate it into a more useful representation.
 * @param buf_raw Buffer containing the raw APDU message.
 * @param buf_raw_len Length of the raw APDU message.
 * @param apdu_cmd Where the parsed APDU will be written.
 * @return Return code.
 */
swicc_ret_et swicc_apdu_cmd_parse(uint8_t const *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  swicc_apdu_cmd_st *const cmd);

/**
 * @brief Produce a response given a response structure. This also validates the
 * contents of the res struct.
 * @param buf_raw Where to write the raw response.
 * @param buf_raw_len Should contain the allocated size of the raw buffer. This
 * will receive the final length of the written response on success.
 * @param cmd The command for which we are creating the response.
 * @param res The response struct.
 * @return Return code.
 */
swicc_ret_et swicc_apdu_res_deparse(uint8_t *const buf_raw,
                                    uint16_t *const buf_raw_len,
                                    swicc_apdu_cmd_st const *const cmd,
                                    swicc_apdu_res_st const *const res);

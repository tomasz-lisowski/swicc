#include "uicc/common.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#pragma once
/**
 * APDU = Application protocol data unit
 */

typedef enum uicc_apdu_cla_ccc_e
{
    UICC_APDU_CLA_CCC_INVALID,
    UICC_APDU_CLA_CCC_LAST,
    UICC_APDU_CLA_CCC_MORE,
} uicc_apdu_cla_ccc_et;

typedef enum uicc_apdu_cla_sm_e
{
    UICC_APDU_CLA_SM_INVALID,      /* Invalid SM indication */
    UICC_APDU_CLA_SM_NO,           /* NO SM or no indication */
    UICC_APDU_CLA_SM_PROPRIETARY,  /* Properietary SM format */
    UICC_APDU_CLA_SM_CMD_HDR_SKIP, /* Std SM and cmd header unsecure */
    UICC_APDU_CLA_SM_CMD_HDR_AUTH, /* Std SM and cmd header secure */
} uicc_apdu_cla_sm_et;

typedef enum uicc_apdu_cla_type_e
{
    UICC_APDU_CLA_TYPE_INVALID,
    UICC_APDU_CLA_TYPE_INTERINDUSTRY,
    UICC_APDU_CLA_TYPE_PROPRIETARY,
    UICC_APDU_CLA_TYPE_RFU, /* Reserved for future use */
} uicc_apdu_cla_type_et;

/**
 * Collection of flags describing all aspects of an APDU command class.
 */
typedef struct uicc_apdu_cla_s
{
    uicc_apdu_cla_ccc_et ccc; /* Command chaining control (CCC) */
    uicc_apdu_cla_sm_et sm;   /* Secure Messaging (SM) indication */
    uicc_apdu_cla_type_et type;
    uint16_t lchan; /* Logical channel number ((CLA >> 27) & 0x0F) */
} uicc_apdu_cla_st;

/**
 * For describing the status word in an APDU response.
 */
typedef enum uicc_apdu_sw_e
{
    UICC_APDU_SW_SUCCESS,
} uicc_apdu_status_et;

/**
 * The header as it comes on the "wire".
 */
typedef struct uicc_apdu_cmd_hdr_raw_s
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
} __attribute__((__packed__)) uicc_apdu_cmd_hdr_raw_st;

/**
 * An internal format of the header that is easier and more efficient to use in
 * code.
 */
typedef struct uicc_apdu_cmd_hdr_s
{
    uicc_apdu_cla_st class;
    uint8_t instruction;
    uint8_t param_1;
    uint8_t param_2;
} uicc_apdu_cmd_hdr_st;

/**
 * An internal format of the APDU command which is the result of parsing a raw
 * APDU command.
 */
typedef struct uicc_apdu_cmd_s
{
    uicc_apdu_cmd_hdr_st hdr;
    uint16_t data_len_cmd;
    uint16_t data_len_res;
    uint8_t data[UICC_DATA_MAX];
} uicc_apdu_cmd_st;

/**
 * An internal format of the APDU response which is the result of parsing a raw
 * APDU response.
 */
typedef struct uicc_apdu_res_s
{
    uint16_t status;
    uint8_t data[UICC_DATA_MAX];
} uicc_apdu_res_st;

typedef uicc_ret_et uicc_apdu_handler_ft(uicc_st *const uicc_state,
                                         uicc_apdu_cmd_st const *const cmd);
/**
 * Store pointers to handlers for every instruction in the interindustry class.
 */
extern uicc_apdu_handler_ft *const uicc_apdu_handler_arr[0xFF + 1U];

/**
 * @brief All APDUs in the proprietary class require non-interindusry
 * implementations for handlers. The handler passed to this function is the
 * function that will get these proprietary messages and is expected to handle
 * them.
 * @param uicc_state
 * @param handler Handler for all proprietary messages.
 * @return Return code.
 * @note Sorry for the long name...
 */
uicc_ret_et uicc_apdu_handler_proprietary_register(
    uicc_st *const uicc_state, uicc_apdu_handler_ft *const handler);

/**
 * @brief Handle all APDUs.
 * @param uicc_state
 * @param cmd Command to handle.
 * @param res Response to the command.
 * @return Return code.
 */
uicc_ret_et uicc_apdu_handle(uicc_st *const uicc_state,
                             uicc_apdu_cmd_st const *const cmd,
                             uicc_apdu_res_st *const res);

/**
 * @brief Parse the raw CLA byte.
 * @param cla_raw The class byte of an APDU message.
 * @return Parsed CLA.
 */
uicc_apdu_cla_st uicc_apdu_cmd_cla_parse(uint8_t const cla_raw);

/**
 * @brief Given a buffer containing a raw interindustry APDU message, parse and
 * validate it into a more useful representation.
 * @param buf_raw Buffer containing the raw APDU message.
 * @param buf_raw_len Length of the buffer containing the raw APDU message.
 * @param apdu_cmd Where the parsed APDU will be written.
 * @return Return code.
 */
uicc_ret_et uicc_apdu_cmd_parse(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                uicc_apdu_cmd_st *const cmd);

uicc_ret_et uicc_apdu_cmd_deparse(uint8_t *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  uicc_apdu_cmd_st const *const cmd);

uicc_ret_et uicc_apdu_res_parse(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                uicc_apdu_res_st *const res);

uicc_ret_et uicc_apdu_res_deparse(uint8_t *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  uicc_apdu_res_st const *const res);

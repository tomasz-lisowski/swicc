#include "usim.h"
#include <stdio.h>

#ifdef DEBUG
/* ISO 7816-4:2020 p.15-16 */
static char const *const usim_dbg_table_str_ins[0xFF] = {
    [APDU_INS_FILE_DEACTIVATE] = "file > deactivate",
    [APDU_INS_FILE_ACTIVATE] = "file > activate",
    [APDU_INS_FILE_CREATE] = "file > create",
    [APDU_INS_FILE_DELETE] = "file > delete",
    [APDU_INS_FILE_DF_TERMINATE] = "file > df > terminate",
    [APDU_INS_FILE_EF_TERMINATE] = "file > ef > terminate",

    [APDU_INS_RECORD_DEACTIVATE] = "record > deactivate",
    [APDU_INS_RECORD_ACTIVATE] = "record > activate",
    [APDU_INS_RECORD_ERASE] = "record > erase",
    [APDU_INS_RECORD_SEARCH] = "record > search",
    [APDU_INS_RECORD_READ0] = "record > read (0)",
    [APDU_INS_RECORD_READ1] = "record > read (1)",
    [APDU_INS_RECORD_WRITE] = "record > write",
    [APDU_INS_RECORD_UPDATE0] = "record > update (0)",
    [APDU_INS_RECORD_UPDATE1] = "record > update (1)",
    [APDU_INS_RECORD_APPEND] = "record > append",

    [APDU_INS_BIN_ERASE0] = "binary > erase (0)",
    [APDU_INS_BIN_ERASE1] = "binary > erase (1)",
    [APDU_INS_BIN_SEARCH0] = "binary > search (0)",
    [APDU_INS_BIN_SEARCH1] = "binary > search (1)",
    [APDU_INS_BIN_READ0] = "binary > read (0)",
    [APDU_INS_BIN_READ1] = "binary > read (1)",
    [APDU_INS_BIN_WRITE0] = "binary > write (0)",
    [APDU_INS_BIN_WRITE1] = "binary > write (1)",
    [APDU_INS_BIN_UPDATE0] = "binary > update (0)",
    [APDU_INS_BIN_UPDATE1] = "binary > update (1)",

    [APDU_INS_OP_SCQL] = "op > scql",
    [APDU_INS_OP_TRANSACTION] = "op > transaction",
    [APDU_INS_OP_USER] = "op > user",
    [APDU_INS_OP_SECURITY0] = "op > security (0)",
    [APDU_INS_OP_SECURITY1] = "op > security (1)",
    [APDU_INS_OP_BIOMETRIC0] = "op > biometric (0)",
    [APDU_INS_OP_BIOMETRIC1] = "op > biometric (1)",

    [APDU_INS_CRYPTO_VERIFY0] = "crypto > verify (0)",
    [APDU_INS_CRYPTO_VERIFY1] = "crypto > verify (1)",
    [APDU_INS_CRYPTO_SECENV_MANAGE] = "crypto > secure environment > manage",
    [APDU_INS_CRYPTO_ASYM_KEY_PAIR_GEN0] =
        "crypto > asymmetric key pair > generate (0)",
    [APDU_INS_CRYPTO_ASYM_KEY_PAIR_GEN1] =
        "crypto > asymmetric key pair > generate (1)",
    [APDU_INS_CRYPTO_CARD_SECRET_IMPORT] = "crypto > card secret > import",
    [APDU_INS_CRYPTO_AUTH_EXTERNAL] = "crypto > authenticate > external",
    [APDU_INS_CRYPTO_CHALLENGE_GET] = "crypto > challenge > get",
    [APDU_INS_CRYPTO_AUTH_GENERAL0] = "crypto > authenticate > general (0)",
    [APDU_INS_CRYPTO_AUTH_GENERAL1] = "crypto > authenticate > general (1)",
    [APDU_INS_CRYPTO_AUTH_INTERNAL] = "crypto > authenticate > internal",
    [APDU_INS_CRYPTO_VERIF_REQ_DISABLE] = "verification requirement > disable",
    [APDU_INS_CRYPTO_VERIF_REQ_ENABLE] = "verification requirement > enable",

    [APDU_INS_APP_MGMT_REQ0] = "app > management request (0)",
    [APDU_INS_APP_MGMT_REQ1] = "app > management request (1)",
    [APDU_INS_APP_LOAD0] = "app > load (0)",
    [APDU_INS_APP_LOAD1] = "app > load (1)",
    [APDU_INS_APP_REMOVE0] = "app > remove (0)",
    [APDU_INS_APP_REMOVE1] = "app > remove (1)",

    [APDU_INS_DATA_SELECT] = "data > select",
    [APDU_INS_DATA_GET0] = "data > get (0)",
    [APDU_INS_DATA_GET1] = "data > get (1)",
    [APDU_INS_DATA_GET2] = "data > get (2)",
    [APDU_INS_DATA_GET3] = "data > get (3)",
    [APDU_INS_DATA_MANAGE] = "data > manage",
    [APDU_INS_DATA_NEXT_PUT0] = "data > next > put (0)",
    [APDU_INS_DATA_NEXT_PUT1] = "data > next > put (1)",
    [APDU_INS_DATA_PUT0] = "data > put (0)",
    [APDU_INS_DATA_PUT1] = "data > put (1)",
    [APDU_INS_DATA_UPDATE0] = "data > update (0)",
    [APDU_INS_DATA_UPDATE1] = "data > update (1)",
    [APDU_INS_DATA_DELETE] = "data > delete",

    [APDU_INS_DEVMGMT0] = "device management (0)",
    [APDU_INS_DEVMGMT1] = "device management (1)",
    [APDU_INS_REFDATA_CHANGE0] = "reference data > change (0)",
    [APDU_INS_REFDATA_CHANGE1] = "reference data > change (1)",
    [APDU_INS_RETRYCTR_RESET0] = "retry counter > reset (0)",
    [APDU_INS_RETRYCTR_RESET1] = "retry counter > reset (1)",
    [APDU_INS_COMPARE] = "compare",
    [APDU_INS_ATTRIB_GET0] = "attribute > get (0)",
    [APDU_INS_ATTRIB_GET1] = "attribute > get (1)",
    [APDU_INS_CHAN_MANAGE] = "channel > manage",
    [APDU_INS_SELECT] = "select",
    [APDU_INS_RES_GET] = "response > get",
    [APDU_INS_ENVELOPE0] = "envelope (0)",
    [APDU_INS_ENVELOPE1] = "envelope (1)",
    [APDU_INS_CREATE] = "create",
    [APDU_INS_CARD_TERMINATE] = "card > terminate",
};

static char const *const usim_dbg_table_str_cla_chain[3U] = {"???", "last",
                                                             "more"};
static char const *const usim_dbg_table_str_cla_sm[5U] = {
    "???", "not indicated", "SM is proprietary",
    "standard but command header is not processed",
    "standard and command header is authenticated"};
static char const *const usim_dbg_table_str_cla_info[5U] = {
    "???", "interindustry", "proprietary", "reserved for future use",
    "invalid"};
#endif

char const *usim_dbg_str_apdu_cla_chain(apdu_cla_et const cla)
{
    uint32_t cla_chain_str_idx = (cla & APDU_CLA_CMD_CHAIN_LAST) +
                                 ((cla & APDU_CLA_CMD_CHAIN_MORE) >> 1U);
    uint8_t arr_el_count = (sizeof(usim_dbg_table_str_cla_chain) /
                            sizeof(usim_dbg_table_str_cla_chain[0U]));
    if (cla_chain_str_idx > arr_el_count - 1U)
    {
        cla_chain_str_idx = 0U;
    }
    return usim_dbg_table_str_cla_chain[cla_chain_str_idx];
}

char const *usim_dbg_str_apdu_cla_sm(apdu_cla_et const cla)
{
    uint32_t cla_sm_str_idx = ((cla & APDU_CLA_SM_NO) >> 2U) +
                              (((cla & APDU_CLA_SM_PROPRIETARY) >> 3U) * 2U) +
                              (((cla & APDU_CLA_SM_CMD_HDR_SKIP) >> 4U) * 3U) +
                              (((cla & APDU_CLA_SM_CMD_HDR_AUTH) >> 5U) * 4U);
    uint8_t arr_el_count = (sizeof(usim_dbg_table_str_cla_sm) /
                            sizeof(usim_dbg_table_str_cla_sm[0U]));
    if (cla_sm_str_idx > arr_el_count - 1U)
    {
        cla_sm_str_idx = 0U;
    }
    return usim_dbg_table_str_cla_sm[cla_sm_str_idx];
}

char const *usim_dbg_str_apdu_cla_info(apdu_cla_et const cla)
{
    uint32_t cla_info_str_idx = ((cla & APDU_CLA_INTERINDUSTRY) >> 23U) +
                                (((cla & APDU_CLA_PROPRIETARY) >> 24U) * 2U) +
                                (((cla & APDU_CLA_RFU) >> 25U) * 3U) +
                                (((cla & APDU_CLA_INVALID) >> 26U) * 4U);
    uint8_t arr_el_count = (sizeof(usim_dbg_table_str_cla_info) /
                            sizeof(usim_dbg_table_str_cla_info[0U]));
    if (cla_info_str_idx > arr_el_count - 1U)
    {
        cla_info_str_idx = 0U;
    }
    return usim_dbg_table_str_cla_info[cla_info_str_idx];
}

uint8_t usim_dbg_str_apdu_cla_lchan(apdu_cla_et const cla)
{
    uint8_t const cla_lchan =
        ((((uint32_t)cla) >> 27) & 0b00001111); /* NOTE: Safe cast */
    return cla_lchan;
}

char const *usim_dbg_str_apdu_ins(apdu_ins_et const ins)
{
    char const *const ins_str_cpy = usim_dbg_table_str_ins[ins];
    return ins_str_cpy == NULL ? "RFU" : ins_str_cpy;
}

usim_ret_et usim_dbg_str_apdu_cmd(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  apdu_cmd_st const *const apdu_cmd)
{
#ifdef DEBUG
    int bytes_written =
        snprintf(buf_str, *buf_str_len,
                 // clang-format off
                 "(APDU"
                 "\n  (CLA (CHAIN '%s') (SM '%s') (INFO '%s') (LCHAN %u))"
                 "\n  (INS OP '%s')"
                 "\n  (P1 0x%x)"
                 "\n  (P2 0x%x))",
                 // clang-format on
                 usim_dbg_str_apdu_cla_chain(apdu_cmd->hdr.class),
                 usim_dbg_str_apdu_cla_sm(apdu_cmd->hdr.class),
                 usim_dbg_str_apdu_cla_info(apdu_cmd->hdr.class),
                 usim_dbg_str_apdu_cla_lchan(apdu_cmd->hdr.class),
                 usim_dbg_str_apdu_ins(apdu_cmd->hdr.instruction),
                 apdu_cmd->hdr.param_1, apdu_cmd->hdr.param_2);
    if (bytes_written < 0)
    {
        return USIM_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        *buf_str_len = (uint16_t)
            bytes_written; /* NOTE: Safe cast due to args of snprintf */
        return USIM_RET_SUCCESS;
    }
#else
    return USIM_RET_SUCCESS;
#endif
}

usim_ret_et usim_dbg_str_apdu_res(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  apdu_res_st const *const apdu_res)
{
#ifdef DEBUG
    return USIM_RET_UNKNOWN;
#else
    return USIM_RET_SUCCESS;
#endif
}

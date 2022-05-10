#include "uicc.h"
#include <stdio.h>

#ifdef DEBUG
/* ISO 7816-4:2020 p.15-16 */
static char const *const uicc_dbg_table_str_ins[0xFF + 1U] = {
    [0x00] = "RFU",
    [0x01] = "RFU",
    [0x02] = "RFU",
    [0x03] = "RFU",
    [0x04] = "file > deactivate",
    [0x05] = "RFU",
    [0x06] = "record > deactivate",
    [0x07] = "RFU",
    [0x08] = "record > activate",
    [0x09] = "RFU",
    [0x0A] = "RFU",
    [0x0B] = "RFU",
    [0x0C] = "record > erase",
    [0x0D] = "RFU",
    [0x0E] = "binary > erase (0)",
    [0x0F] = "binary > erase (1)",
    [0x10] = "op > SCQL",
    [0x11] = "RFU",
    [0x12] = "op > transaction",
    [0x13] = "RFU",
    [0x14] = "op > user",
    [0x15] = "RFU",
    [0x16] = "dev > mgmt (0)",
    [0x17] = "dev > mgmt (1)",
    [0x18] = "RFU",
    [0x19] = "RFU",
    [0x1A] = "RFU",
    [0x1B] = "RFU",
    [0x1C] = "RFU",
    [0x1D] = "RFU",
    [0x1E] = "RFU",
    [0x1F] = "RFU",
    [0x20] = "refdata > verify (0)",
    [0x21] = "refdata > verify (1)",
    [0x22] = "secenv > mgmt",
    [0x23] = "RFU",
    [0x24] = "refdata > change (0)",
    [0x25] = "refdata > change (1)",
    [0x26] = "refdata > disable",
    [0x27] = "RFU",
    [0x28] = "refdata > enable",
    [0x29] = "RFU",
    [0x2A] = "op > security (0)",
    [0x2B] = "op > security (1)",
    [0x2C] = "refdata > counter > reset (0)",
    [0x2D] = "refdata > counter > reset (1)",
    [0x2E] = "op > biometric (0)",
    [0x2F] = "op > biometric (1)",
    [0x30] = "RFU",
    [0x31] = "RFU",
    [0x32] = "RFU",
    [0x33] = "compare",
    [0x34] = "attrib > get (0)",
    [0x35] = "attrib > get (1)",
    [0x36] = "RFU",
    [0x37] = "RFU",
    [0x38] = "RFU",
    [0x39] = "RFU",
    [0x3A] = "RFU",
    [0x3B] = "RFU",
    [0x3C] = "RFU",
    [0x3D] = "RFU",
    [0x3E] = "RFU",
    [0x3F] = "RFU",
    [0x40] = "app > mgmt (0)",
    [0x41] = "app > mgmt (1)",
    [0x42] = "RFU",
    [0x43] = "RFU",
    [0x44] = "file > activate",
    [0x45] = "RFU",
    [0x46] = "crypto > asym > gen key pair (0)",
    [0x47] = "crypto > asym > gen key pair (1)",
    [0x48] = "crypto > import > secret",
    [0x49] = "RFU",
    [0x4A] = "RFU",
    [0x4B] = "RFU",
    [0x4C] = "RFU",
    [0x4D] = "RFU",
    [0x4E] = "RFU",
    [0x4F] = "RFU",
    [0x50] = "RFU",
    [0x51] = "RFU",
    [0x52] = "RFU",
    [0x53] = "RFU",
    [0x54] = "RFU",
    [0x55] = "RFU",
    [0x56] = "RFU",
    [0x57] = "RFU",
    [0x58] = "RFU",
    [0x59] = "RFU",
    [0x5A] = "RFU",
    [0x5B] = "RFU",
    [0x5C] = "RFU",
    [0x5D] = "RFU",
    [0x5E] = "RFU",
    [0x5F] = "RFU",
    [0x60] = "RFU",
    [0x61] = "RFU",
    [0x62] = "RFU",
    [0x63] = "RFU",
    [0x64] = "RFU",
    [0x65] = "RFU",
    [0x66] = "RFU",
    [0x67] = "RFU",
    [0x68] = "RFU",
    [0x69] = "RFU",
    [0x6A] = "RFU",
    [0x6B] = "RFU",
    [0x6C] = "RFU",
    [0x6D] = "RFU",
    [0x6E] = "RFU",
    [0x6F] = "RFU",
    [0x70] = "ch > mgmt",
    [0x71] = "RFU",
    [0x72] = "RFU",
    [0x73] = "RFU",
    [0x74] = "RFU",
    [0x75] = "RFU",
    [0x76] = "RFU",
    [0x77] = "RFU",
    [0x78] = "RFU",
    [0x79] = "RFU",
    [0x7A] = "RFU",
    [0x7B] = "RFU",
    [0x7C] = "RFU",
    [0x7D] = "RFU",
    [0x7E] = "RFU",
    [0x7F] = "RFU",
    [0x80] = "RFU",
    [0x81] = "RFU",
    [0x82] = "auth > external",
    [0x83] = "RFU",
    [0x84] = "challenge > get",
    [0x85] = "RFU",
    [0x86] = "auth > general (0)",
    [0x87] = "auth > general (1)",
    [0x88] = "auth > internal",
    [0x89] = "RFU",
    [0x8A] = "RFU",
    [0x8B] = "RFU",
    [0x8C] = "RFU",
    [0x8D] = "RFU",
    [0x8E] = "RFU",
    [0x8F] = "RFU",
    [0x90] = "RFU",
    [0x91] = "RFU",
    [0x92] = "RFU",
    [0x93] = "RFU",
    [0x94] = "RFU",
    [0x95] = "RFU",
    [0x96] = "RFU",
    [0x97] = "RFU",
    [0x98] = "RFU",
    [0x99] = "RFU",
    [0x9A] = "RFU",
    [0x9B] = "RFU",
    [0x9C] = "RFU",
    [0x9D] = "RFU",
    [0x9E] = "RFU",
    [0x9F] = "RFU",
    [0xA0] = "binary > search (0)",
    [0xA1] = "binary > search (1)",
    [0xA2] = "record > search",
    [0xA3] = "RFU",
    [0xA4] = "file > select",
    [0xA5] = "data > select",
    [0xA6] = "RFU",
    [0xA7] = "RFU",
    [0xA8] = "RFU",
    [0xA9] = "RFU",
    [0xAA] = "RFU",
    [0xAB] = "RFU",
    [0xAC] = "RFU",
    [0xAD] = "RFU",
    [0xAE] = "RFU",
    [0xAF] = "RFU",
    [0xB0] = "binary > read (0)",
    [0xB1] = "binary > read (1)",
    [0xB2] = "record > read (0)",
    [0xB3] = "record > read (1)",
    [0xB4] = "RFU",
    [0xB5] = "RFU",
    [0xB6] = "RFU",
    [0xB7] = "RFU",
    [0xB8] = "RFU",
    [0xB9] = "RFU",
    [0xBA] = "RFU",
    [0xBB] = "RFU",
    [0xBC] = "RFU",
    [0xBD] = "RFU",
    [0xBE] = "RFU",
    [0xBF] = "RFU",
    [0xC0] = "res > get",
    [0xC1] = "RFU",
    [0xC2] = "envelope (0)",
    [0xC3] = "envelope (1)",
    [0xC4] = "RFU",
    [0xC5] = "RFU",
    [0xC6] = "RFU",
    [0xC7] = "RFU",
    [0xC8] = "RFU",
    [0xC9] = "RFU",
    [0xCA] = "data > get/next (0)",
    [0xCB] = "data > get/next (1)",
    [0xCC] = "data > get/next (2)",
    [0xCD] = "data > get/next (3)",
    [0xCE] = "RFU",
    [0xCF] = "data > mgmt",
    [0xD0] = "binary > write (0)",
    [0xD1] = "binary > write (1)",
    [0xD2] = "record > write",
    [0xD3] = "RFU",
    [0xD4] = "RFU",
    [0xD5] = "RFU",
    [0xD6] = "binary > update (0)",
    [0xD7] = "binary > update (1)",
    [0xD8] = "data > put next (0)",
    [0xD9] = "data > put next (1)",
    [0xDA] = "data > put (0)",
    [0xDB] = "data > put (1)",
    [0xDC] = "record > update (0)",
    [0xDD] = "record > update (1)",
    [0xDE] = "data > update (0)",
    [0xDF] = "data > update (1)",
    [0xE0] = "file > create",
    [0xE1] = "create",
    [0xE2] = "record > append",
    [0xE3] = "RFU",
    [0xE4] = "file > delete",
    [0xE5] = "RFU",
    [0xE6] = "file > df > terminate",
    [0xE7] = "RFU",
    [0xE8] = "file > ef > terminate",
    [0xE9] = "RFU",
    [0xEA] = "app > load (0)",
    [0xEB] = "app > load (1)",
    [0xEC] = "app > remove (0)",
    [0xED] = "app > remove (1)",
    [0xEE] = "data > delete",
    [0xEF] = "RFU",
    [0xF0] = "RFU",
    [0xF1] = "RFU",
    [0xF2] = "RFU",
    [0xF3] = "RFU",
    [0xF4] = "RFU",
    [0xF5] = "RFU",
    [0xF6] = "RFU",
    [0xF7] = "RFU",
    [0xF8] = "RFU",
    [0xF9] = "RFU",
    [0xFA] = "RFU",
    [0xFB] = "RFU",
    [0xFC] = "RFU",
    [0xFD] = "RFU",
    [0xFE] = "card > terminate",
    [0xFF] = "RFU",
};

static char const *const uicc_dbg_table_str_cla_chain[3U] = {
    [UICC_APDU_CLA_CCC_INVALID] = "???",
    [UICC_APDU_CLA_CCC_LAST] = "last",
    [UICC_APDU_CLA_CCC_MORE] = "more",
};
static char const *const uicc_dbg_table_str_cla_sm[5U] = {
    [UICC_APDU_CLA_SM_INVALID] = "???",
    [UICC_APDU_CLA_SM_NO] = "not indicated",
    [UICC_APDU_CLA_SM_PROPRIETARY] = "SM is proprietary",
    [UICC_APDU_CLA_SM_CMD_HDR_SKIP] =
        "standard but command header is not processed",
    [UICC_APDU_CLA_SM_CMD_HDR_AUTH] =
        "standard and command header is authenticated",
};
static char const *const uicc_dbg_table_str_cla_info[5U] = {
    [UICC_APDU_CLA_TYPE_INVALID] = "???",
    [UICC_APDU_CLA_TYPE_INTERINDUSTRY] = "interindustry",
    [UICC_APDU_CLA_TYPE_PROPRIETARY] = "proprietary",
    [UICC_APDU_CLA_TYPE_RFU] = "reserved for future use",
};
#endif /* DEBUG */

char const *uicc_dbg_apdu_cla_ccc_str(uicc_apdu_cla_st const cla)
{
#ifdef DEBUG
    uint8_t cla_chain_str_idx =
        (uint8_t)cla.ccc; /* Safe case, all enum members are positive. */
    uint8_t arr_el_count = (sizeof(uicc_dbg_table_str_cla_chain) /
                            sizeof(uicc_dbg_table_str_cla_chain[0U]));
    if (cla_chain_str_idx > arr_el_count - 1)
    {
        cla_chain_str_idx = UICC_APDU_CLA_CCC_INVALID;
    }
    return uicc_dbg_table_str_cla_chain[cla_chain_str_idx];
#else
    return NULL;
#endif
}

char const *uicc_dbg_apdu_cla_sm_str(uicc_apdu_cla_st const cla)
{
#ifdef DEBUG
    uint32_t cla_sm_str_idx =
        (uint8_t)cla.sm; /* Safe case, all enum members are positive. */
    uint8_t arr_el_count = (sizeof(uicc_dbg_table_str_cla_sm) /
                            sizeof(uicc_dbg_table_str_cla_sm[0U]));
    if (cla_sm_str_idx > arr_el_count - 1U)
    {
        cla_sm_str_idx = UICC_APDU_CLA_SM_INVALID;
    }
    return uicc_dbg_table_str_cla_sm[cla_sm_str_idx];
#else
    return NULL;
#endif
}

char const *uicc_dbg_apdu_cla_type_str(uicc_apdu_cla_st const cla)
{
#ifdef DEBUG
    uint32_t cla_info_str_idx =
        (uint8_t)cla.type; /* Safe case, all enum members are positive. */
    uint8_t arr_el_count = (sizeof(uicc_dbg_table_str_cla_info) /
                            sizeof(uicc_dbg_table_str_cla_info[0U]));
    if (cla_info_str_idx > arr_el_count - 1U)
    {
        cla_info_str_idx = UICC_APDU_CLA_TYPE_INVALID;
    }
    return uicc_dbg_table_str_cla_info[cla_info_str_idx];
#else
    return NULL;
#endif
}

char const *uicc_dbg_apdu_ins_str(uint8_t const ins)
{
#ifdef DEBUG
    char const *const ins_str_cpy = uicc_dbg_table_str_ins[ins];
    return ins_str_cpy == NULL ? "???" : ins_str_cpy;
#else
    return NULL;
#endif
}

uicc_ret_et uicc_dbg_apdu_cmd_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_apdu_cmd_st const *const apdu_cmd)
{
#ifdef DEBUG
    int bytes_written =
        snprintf(buf_str, *buf_str_len,
                 // clang-format off
                 "(APDU"
                 "\n  (CLA (CHAIN '%s') (SM '%s') (INFO '%s') (LCHAN %u))"
                 "\n  (INS OP '%s')"
                 "\n  (P1 0x%02X)"
                 "\n  (P2 0x%02X))",
                 // clang-format on
                 uicc_dbg_apdu_cla_ccc_str(apdu_cmd->hdr->cla),
                 uicc_dbg_apdu_cla_sm_str(apdu_cmd->hdr->cla),
                 uicc_dbg_apdu_cla_type_str(apdu_cmd->hdr->cla),
                 apdu_cmd->hdr->cla.lchan,
                 apdu_cmd->hdr->cla.type == UICC_APDU_CLA_TYPE_INTERINDUSTRY
                     ? uicc_dbg_apdu_ins_str(apdu_cmd->hdr->ins)
                     : "???",
                 apdu_cmd->hdr->p1, apdu_cmd->hdr->p2);
    if (bytes_written < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        *buf_str_len =
            (uint16_t)bytes_written; /* Safe cast due to args of snprintf */
        return UICC_RET_SUCCESS;
    }
#else
    return UICC_RET_SUCCESS;
#endif
}

uicc_ret_et uicc_dbg_apdu_res_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_apdu_res_st const *const apdu_res)
{
#ifdef DEBUG
    return UICC_RET_UNKNOWN;
#else
    return UICC_RET_SUCCESS;
#endif
}

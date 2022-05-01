#include "uicc.h"
#include <string.h>

/**
 * @brief Handle both invalid and unknown instructions.
 * @param uicc_state
 * @param cmd
 * @param res
 * @return Return code.
 */
static uicc_ret_et apdu_h_unk(__attribute__((unused)) uicc_st *const uicc_state,
                              __attribute__((unused))
                              uicc_apdu_cmd_st const *const cmd,
                              uicc_apdu_res_st *const res)
{
    res->sw1 = UICC_APDU_SW1_CHER_INS;
    res->sw2 = 0;
    res->data.len = 0;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_apdu_handle_pro_register(uicc_st *const uicc_state,
                                          uicc_apdu_h_ft *const handler)
{
    uicc_state->internal.handle_pro = handler;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_apdu_handle(uicc_st *const uicc_state,
                             uicc_apdu_cmd_st const *const cmd,
                             uicc_apdu_res_st *const res)
{
    uicc_ret_et ret = UICC_RET_APDU_UNHANDLED;
    switch (cmd->hdr->cla.type)
    {
    case UICC_APDU_CLA_TYPE_INVALID:
    case UICC_APDU_CLA_TYPE_RFU:
        res->sw1 = UICC_APDU_SW1_CHER_CLA; /* Marked as unsupported class. */
        res->sw2 = 0;
        res->data.len = 0;
        ret = UICC_RET_SUCCESS;
        break;
    case UICC_APDU_CLA_TYPE_INTERINDUSTRY:
        ret = uicc_apdu_h[cmd->hdr->ins](uicc_state, cmd, res);
        break;
    case UICC_APDU_CLA_TYPE_PROPRIETARY:
        if (uicc_state->internal.handle_pro == NULL)
        {
            ret = UICC_RET_APDU_UNHANDLED;
            break;
        }
        return uicc_state->internal.handle_pro(uicc_state, cmd, res);
    default:
        ret = UICC_RET_APDU_UNHANDLED;
        break;
    }

    if (ret == UICC_RET_APDU_UNHANDLED)
    {
        ret = UICC_RET_SUCCESS;
        res->sw1 = UICC_APDU_SW1_CHER_INS;
        res->sw2 = 0;
        res->data.len = 0;
    }
    return ret;
}

uicc_apdu_cla_st uicc_apdu_cmd_cla_parse(uint8_t const cla_raw)
{
    uicc_apdu_cla_st cla = {0};
    if (cla_raw >> (8U - 3U) == 0b000U) /* ISO 7816-4:2020 p.13 table.2 */
    {
        cla.lchan = cla_raw & 0b00000011U;
        cla.type = UICC_APDU_CLA_TYPE_INTERINDUSTRY;
        cla.ccc = (cla_raw & 0b00010000U) == 0U ? UICC_APDU_CLA_CCC_LAST
                                                : UICC_APDU_CLA_CCC_MORE;
        switch ((cla_raw & 0b00001100) >> 2)
        {
        case 0b00:
            cla.sm = UICC_APDU_CLA_SM_NO;
            break;
        case 0b01:
            cla.sm = UICC_APDU_CLA_SM_PROPRIETARY;
            break;
        case 0b10:
            cla.sm = UICC_APDU_CLA_SM_CMD_HDR_SKIP;
            break;
        case 0b11:
            cla.sm = UICC_APDU_CLA_SM_CMD_HDR_AUTH;
            break;
        default:
            cla.sm = UICC_APDU_CLA_SM_INVALID;
            break;
        }
    }
    else if (cla_raw >> (8U - 2U) == 0b01U) /* ISO 7816-4:2020 p.13 table.3 */
    {
        cla.lchan =
            (uint8_t)((cla_raw & 0b00001111U) + 4U); /* ISO 7816-4:2020 p.13 */
        cla.type = UICC_APDU_CLA_TYPE_INTERINDUSTRY;
        cla.ccc = (cla_raw & 0b00010000U) == 0U ? UICC_APDU_CLA_CCC_LAST
                                                : UICC_APDU_CLA_CCC_MORE;
        cla.sm = (cla_raw & 0b00100000U) == 0U ? UICC_APDU_CLA_SM_NO
                                               : UICC_APDU_CLA_SM_CMD_HDR_SKIP;
    }
    else if (cla_raw >> (8U - 3U) == 0b001U) /* ISO 7816-4:2020 p.12 */
    {
        cla.type = UICC_APDU_CLA_TYPE_RFU;
    }
    else if (cla_raw >> (8U - 4U) ==
                 0b1010U || /* ETSI TS 102 221 V16.4.0 p.76 */
             cla_raw >> (8U - 4U) ==
                 0b1000U /* ETSI TS 102 221 V16.4.0 p.76 and GSM 11.11 4.21.1
                            pg.32 sec.9.1.*/
    )
    {
        cla.type = UICC_APDU_CLA_TYPE_PROPRIETARY;
    }
    else
    {
        cla.type = UICC_APDU_CLA_TYPE_INVALID;
    }
    return cla;
}

uicc_ret_et uicc_apdu_cmd_parse(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                uicc_apdu_cmd_st *const cmd)
{
    if (buf_raw_len < sizeof(uicc_apdu_cmd_hdr_raw_st) ||
        buf_raw_len > sizeof(uicc_apdu_cmd_hdr_raw_st) + UICC_DATA_MAX)
    {
        return UICC_RET_APDU_HDR_TOO_SHORT;
    }

    memset(cmd, 0, sizeof(uicc_apdu_cmd_st));
    cmd->hdr->cla = uicc_apdu_cmd_cla_parse(buf_raw[0]);
    cmd->hdr->ins = buf_raw[1U];
    cmd->hdr->p1 = buf_raw[2U];
    cmd->hdr->p2 = buf_raw[3U];
    cmd->data->len =
        (uint16_t)(buf_raw_len -
                   sizeof(uicc_apdu_cmd_hdr_raw_st)); /* Safe cast due to checks
                                                         at start. */
    memcpy(cmd->data->b, &buf_raw[sizeof(uicc_apdu_cmd_hdr_raw_st)],
           cmd->data->len);
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_apdu_res_deparse(uint8_t *const buf_raw,
                                  uint16_t *const buf_raw_len,
                                  uicc_apdu_cmd_st const *const cmd,
                                  uicc_apdu_res_st const *const res)
{
    /* Ensure there is space in the raw buffers for the whole response. */
    if (*buf_raw_len <
        res->data.len + (res->sw1 == UICC_APDU_SW1_PROC_NULL ||
                                 res->sw1 == UICC_APDU_SW1_PROC_ACK
                             ? 1U
                             : 2U))
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    if (res->data.len > UICC_DATA_MAX)
    {
        /* Unexpected. */
        return UICC_RET_UNKNOWN;
    }
    *buf_raw_len = res->data.len;
    memcpy(buf_raw, res->data.b, res->data.len);
    uint16_t *const status =
        (uint16_t *)&buf_raw[res->data.len]; /* Safe cast because size of raw
                                                buffer was checked. */

    static uint16_t const sw1_raw[] = {
        [UICC_APDU_SW1_NORM_NONE] = 0x9000,
        [UICC_APDU_SW1_NORM_BYTES_AVAILABLE] = 0x6100,
        [UICC_APDU_SW1_WARN_NVM_CHGN] = 0x6200,
        [UICC_APDU_SW1_WARN_NVM_CHGM] = 0x6300,
        [UICC_APDU_SW1_EXER_NVM_CHGN] = 0x6400,
        [UICC_APDU_SW1_EXER_NVM_CHGM] = 0x6500,
        [UICC_APDU_SW1_EXER_SEC] = 0x6600,
        [UICC_APDU_SW1_CHER_LEN] = 0x6700,
        [UICC_APDU_SW1_CHER_CLA_FUNC] = 0x6800,
        [UICC_APDU_SW1_CHER_CMD] = 0x6900,
        [UICC_APDU_SW1_CHER_P1P2_INFO] = 0x6A00,
        [UICC_APDU_SW1_CHER_P1P2] = 0x6B00,
        [UICC_APDU_SW1_CHER_LE] = 0x6C00,
        [UICC_APDU_SW1_CHER_INS] = 0x6D00,
        [UICC_APDU_SW1_CHER_CLA] = 0x6E00,
        [UICC_APDU_SW1_CHER_UNK] = 0x6F00,
    };

    switch (res->sw1)
    {
    case UICC_APDU_SW1_NORM_NONE:
    case UICC_APDU_SW1_CHER_P1P2:
    case UICC_APDU_SW1_CHER_INS:
    case UICC_APDU_SW1_CHER_CLA:
    case UICC_APDU_SW1_CHER_UNK:
        if (res->sw2 != 0)
        {
            return UICC_RET_APDU_RES_INVALID;
        }
        /* Only the above SW1 bytes need a check of SW2 begin 0. */
        __attribute__((fallthrough));
    case UICC_APDU_SW1_NORM_BYTES_AVAILABLE:
    case UICC_APDU_SW1_WARN_NVM_CHGN:
    case UICC_APDU_SW1_WARN_NVM_CHGM:
    case UICC_APDU_SW1_EXER_NVM_CHGN:
    case UICC_APDU_SW1_EXER_NVM_CHGM:
    case UICC_APDU_SW1_EXER_SEC:
    case UICC_APDU_SW1_CHER_LEN:
    case UICC_APDU_SW1_CHER_CLA_FUNC:
    case UICC_APDU_SW1_CHER_CMD:
    case UICC_APDU_SW1_CHER_P1P2_INFO:
    case UICC_APDU_SW1_CHER_LE:
        *status =
            (uint16_t)(sw1_raw[res->sw1] |
                       res->sw2); /* Safe cast because literally impossible to
                                     overflow on bitwise OR. */
        *buf_raw_len = (uint16_t)(*buf_raw_len +
                                  2U); /* Safe cast due to check at the
                                          start that ensures res will fit. */
        return UICC_RET_SUCCESS;
    case UICC_APDU_SW1_PROC_NULL:
        if (res->data.len != 0 || res->sw2 != 0)
        {
            return UICC_RET_APDU_RES_INVALID;
        }
        buf_raw[0U] = 0x60;
        *buf_raw_len = 1;
        return UICC_RET_SUCCESS;
    case UICC_APDU_SW1_PROC_ACK:
        if (res->data.len != 0 || res->sw2 != 0)
        {
            return UICC_RET_APDU_RES_INVALID;
        }
        /* NOTE: Can also be INS XOR 0xFF to get the next (one) byte but this is
         * not supported in this implementation. Older 7816-3 standard editions
         * defined two other ways that have been deprecated now. */
        buf_raw[0U] = cmd->hdr->ins; /* The raw INS byte. */
        *buf_raw_len = 1;
        return UICC_RET_APDU_DATA_WAIT;
    }
    return UICC_RET_SUCCESS;
}

uicc_apdu_h_ft *const uicc_apdu_h[0xFF + 1U] = {
    [0x00] = apdu_h_unk, [0x01] = apdu_h_unk, [0x02] = apdu_h_unk,
    [0x03] = apdu_h_unk, [0x04] = apdu_h_unk, [0x05] = apdu_h_unk,
    [0x06] = apdu_h_unk, [0x07] = apdu_h_unk, [0x08] = apdu_h_unk,
    [0x09] = apdu_h_unk, [0x0A] = apdu_h_unk, [0x0B] = apdu_h_unk,
    [0x0C] = apdu_h_unk, [0x0D] = apdu_h_unk, [0x0E] = apdu_h_unk,
    [0x0F] = apdu_h_unk, [0x10] = apdu_h_unk, [0x11] = apdu_h_unk,
    [0x12] = apdu_h_unk, [0x13] = apdu_h_unk, [0x14] = apdu_h_unk,
    [0x15] = apdu_h_unk, [0x16] = apdu_h_unk, [0x17] = apdu_h_unk,
    [0x18] = apdu_h_unk, [0x19] = apdu_h_unk, [0x1A] = apdu_h_unk,
    [0x1B] = apdu_h_unk, [0x1C] = apdu_h_unk, [0x1D] = apdu_h_unk,
    [0x1E] = apdu_h_unk, [0x1F] = apdu_h_unk, [0x20] = apdu_h_unk,
    [0x21] = apdu_h_unk, [0x22] = apdu_h_unk, [0x23] = apdu_h_unk,
    [0x24] = apdu_h_unk, [0x25] = apdu_h_unk, [0x26] = apdu_h_unk,
    [0x27] = apdu_h_unk, [0x28] = apdu_h_unk, [0x29] = apdu_h_unk,
    [0x2A] = apdu_h_unk, [0x2B] = apdu_h_unk, [0x2C] = apdu_h_unk,
    [0x2D] = apdu_h_unk, [0x2E] = apdu_h_unk, [0x2F] = apdu_h_unk,
    [0x30] = apdu_h_unk, [0x31] = apdu_h_unk, [0x32] = apdu_h_unk,
    [0x33] = apdu_h_unk, [0x34] = apdu_h_unk, [0x35] = apdu_h_unk,
    [0x36] = apdu_h_unk, [0x37] = apdu_h_unk, [0x38] = apdu_h_unk,
    [0x39] = apdu_h_unk, [0x3A] = apdu_h_unk, [0x3B] = apdu_h_unk,
    [0x3C] = apdu_h_unk, [0x3D] = apdu_h_unk, [0x3E] = apdu_h_unk,
    [0x3F] = apdu_h_unk, [0x40] = apdu_h_unk, [0x41] = apdu_h_unk,
    [0x42] = apdu_h_unk, [0x43] = apdu_h_unk, [0x44] = apdu_h_unk,
    [0x45] = apdu_h_unk, [0x46] = apdu_h_unk, [0x47] = apdu_h_unk,
    [0x48] = apdu_h_unk, [0x49] = apdu_h_unk, [0x4A] = apdu_h_unk,
    [0x4B] = apdu_h_unk, [0x4C] = apdu_h_unk, [0x4D] = apdu_h_unk,
    [0x4E] = apdu_h_unk, [0x4F] = apdu_h_unk, [0x50] = apdu_h_unk,
    [0x51] = apdu_h_unk, [0x52] = apdu_h_unk, [0x53] = apdu_h_unk,
    [0x54] = apdu_h_unk, [0x55] = apdu_h_unk, [0x56] = apdu_h_unk,
    [0x57] = apdu_h_unk, [0x58] = apdu_h_unk, [0x59] = apdu_h_unk,
    [0x5A] = apdu_h_unk, [0x5B] = apdu_h_unk, [0x5C] = apdu_h_unk,
    [0x5D] = apdu_h_unk, [0x5E] = apdu_h_unk, [0x5F] = apdu_h_unk,
    [0x60] = apdu_h_unk, [0x61] = apdu_h_unk, [0x62] = apdu_h_unk,
    [0x63] = apdu_h_unk, [0x64] = apdu_h_unk, [0x65] = apdu_h_unk,
    [0x66] = apdu_h_unk, [0x67] = apdu_h_unk, [0x68] = apdu_h_unk,
    [0x69] = apdu_h_unk, [0x6A] = apdu_h_unk, [0x6B] = apdu_h_unk,
    [0x6C] = apdu_h_unk, [0x6D] = apdu_h_unk, [0x6E] = apdu_h_unk,
    [0x6F] = apdu_h_unk, [0x70] = apdu_h_unk, [0x71] = apdu_h_unk,
    [0x72] = apdu_h_unk, [0x73] = apdu_h_unk, [0x74] = apdu_h_unk,
    [0x75] = apdu_h_unk, [0x76] = apdu_h_unk, [0x77] = apdu_h_unk,
    [0x78] = apdu_h_unk, [0x79] = apdu_h_unk, [0x7A] = apdu_h_unk,
    [0x7B] = apdu_h_unk, [0x7C] = apdu_h_unk, [0x7D] = apdu_h_unk,
    [0x7E] = apdu_h_unk, [0x7F] = apdu_h_unk, [0x80] = apdu_h_unk,
    [0x81] = apdu_h_unk, [0x82] = apdu_h_unk, [0x83] = apdu_h_unk,
    [0x84] = apdu_h_unk, [0x85] = apdu_h_unk, [0x86] = apdu_h_unk,
    [0x87] = apdu_h_unk, [0x88] = apdu_h_unk, [0x89] = apdu_h_unk,
    [0x8A] = apdu_h_unk, [0x8B] = apdu_h_unk, [0x8C] = apdu_h_unk,
    [0x8D] = apdu_h_unk, [0x8E] = apdu_h_unk, [0x8F] = apdu_h_unk,
    [0x90] = apdu_h_unk, [0x91] = apdu_h_unk, [0x92] = apdu_h_unk,
    [0x93] = apdu_h_unk, [0x94] = apdu_h_unk, [0x95] = apdu_h_unk,
    [0x96] = apdu_h_unk, [0x97] = apdu_h_unk, [0x98] = apdu_h_unk,
    [0x99] = apdu_h_unk, [0x9A] = apdu_h_unk, [0x9B] = apdu_h_unk,
    [0x9C] = apdu_h_unk, [0x9D] = apdu_h_unk, [0x9E] = apdu_h_unk,
    [0x9F] = apdu_h_unk, [0xA0] = apdu_h_unk, [0xA1] = apdu_h_unk,
    [0xA2] = apdu_h_unk, [0xA3] = apdu_h_unk, [0xA4] = apdu_h_unk,
    [0xA5] = apdu_h_unk, [0xA6] = apdu_h_unk, [0xA7] = apdu_h_unk,
    [0xA8] = apdu_h_unk, [0xA9] = apdu_h_unk, [0xAA] = apdu_h_unk,
    [0xAB] = apdu_h_unk, [0xAC] = apdu_h_unk, [0xAD] = apdu_h_unk,
    [0xAE] = apdu_h_unk, [0xAF] = apdu_h_unk, [0xB0] = apdu_h_unk,
    [0xB1] = apdu_h_unk, [0xB2] = apdu_h_unk, [0xB3] = apdu_h_unk,
    [0xB4] = apdu_h_unk, [0xB5] = apdu_h_unk, [0xB6] = apdu_h_unk,
    [0xB7] = apdu_h_unk, [0xB8] = apdu_h_unk, [0xB9] = apdu_h_unk,
    [0xBA] = apdu_h_unk, [0xBB] = apdu_h_unk, [0xBC] = apdu_h_unk,
    [0xBD] = apdu_h_unk, [0xBE] = apdu_h_unk, [0xBF] = apdu_h_unk,
    [0xC0] = apdu_h_unk, [0xC1] = apdu_h_unk, [0xC2] = apdu_h_unk,
    [0xC3] = apdu_h_unk, [0xC4] = apdu_h_unk, [0xC5] = apdu_h_unk,
    [0xC6] = apdu_h_unk, [0xC7] = apdu_h_unk, [0xC8] = apdu_h_unk,
    [0xC9] = apdu_h_unk, [0xCA] = apdu_h_unk, [0xCB] = apdu_h_unk,
    [0xCC] = apdu_h_unk, [0xCD] = apdu_h_unk, [0xCE] = apdu_h_unk,
    [0xCF] = apdu_h_unk, [0xD0] = apdu_h_unk, [0xD1] = apdu_h_unk,
    [0xD2] = apdu_h_unk, [0xD3] = apdu_h_unk, [0xD4] = apdu_h_unk,
    [0xD5] = apdu_h_unk, [0xD6] = apdu_h_unk, [0xD7] = apdu_h_unk,
    [0xD8] = apdu_h_unk, [0xD9] = apdu_h_unk, [0xDA] = apdu_h_unk,
    [0xDB] = apdu_h_unk, [0xDC] = apdu_h_unk, [0xDD] = apdu_h_unk,
    [0xDE] = apdu_h_unk, [0xDF] = apdu_h_unk, [0xE0] = apdu_h_unk,
    [0xE1] = apdu_h_unk, [0xE2] = apdu_h_unk, [0xE3] = apdu_h_unk,
    [0xE4] = apdu_h_unk, [0xE5] = apdu_h_unk, [0xE6] = apdu_h_unk,
    [0xE7] = apdu_h_unk, [0xE8] = apdu_h_unk, [0xE9] = apdu_h_unk,
    [0xEA] = apdu_h_unk, [0xEB] = apdu_h_unk, [0xEC] = apdu_h_unk,
    [0xED] = apdu_h_unk, [0xEE] = apdu_h_unk, [0xEF] = apdu_h_unk,
    [0xF0] = apdu_h_unk, [0xF1] = apdu_h_unk, [0xF2] = apdu_h_unk,
    [0xF3] = apdu_h_unk, [0xF4] = apdu_h_unk, [0xF5] = apdu_h_unk,
    [0xF6] = apdu_h_unk, [0xF7] = apdu_h_unk, [0xF8] = apdu_h_unk,
    [0xF9] = apdu_h_unk, [0xFA] = apdu_h_unk, [0xFB] = apdu_h_unk,
    [0xFC] = apdu_h_unk, [0xFD] = apdu_h_unk, [0xFE] = apdu_h_unk,
    [0xFF] = apdu_h_unk,
};

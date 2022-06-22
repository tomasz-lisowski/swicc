#include <string.h>
#include <swicc/swicc.h>

swicc_apdu_cla_st swicc_apdu_cmd_cla_parse(uint8_t const cla_raw)
{
    swicc_apdu_cla_st cla = {0U};
    cla.raw = cla_raw;
    if (cla_raw >> (8U - 3U) == 0b000U) /* ISO 7816-4:2020 p.13 table.2 */
    {
        cla.lchan = cla_raw & 0b00000011U;
        cla.type = SWICC_APDU_CLA_TYPE_INTERINDUSTRY;
        cla.ccc = (cla_raw & 0b00010000U) == 0U ? SWICC_APDU_CLA_CCC_LAST
                                                : SWICC_APDU_CLA_CCC_MORE;
        switch ((cla_raw & 0b00001100) >> 2)
        {
        case 0b00:
            cla.sm = SWICC_APDU_CLA_SM_NO;
            break;
        case 0b01:
            cla.sm = SWICC_APDU_CLA_SM_PROPRIETARY;
            break;
        case 0b10:
            cla.sm = SWICC_APDU_CLA_SM_CMD_HDR_SKIP;
            break;
        case 0b11:
            cla.sm = SWICC_APDU_CLA_SM_CMD_HDR_AUTH;
            break;
        default:
            cla.sm = SWICC_APDU_CLA_SM_INVALID;
            break;
        }
    }
    else if (cla_raw >> (8U - 2U) == 0b01U) /* ISO 7816-4:2020 p.13 table.3 */
    {
        cla.lchan =
            (uint8_t)((cla_raw & 0b00001111U) + 4U); /* ISO 7816-4:2020 p.13 */
        cla.type = SWICC_APDU_CLA_TYPE_INTERINDUSTRY;
        cla.ccc = (cla_raw & 0b00010000U) == 0U ? SWICC_APDU_CLA_CCC_LAST
                                                : SWICC_APDU_CLA_CCC_MORE;
        cla.sm = (cla_raw & 0b00100000U) == 0U ? SWICC_APDU_CLA_SM_NO
                                               : SWICC_APDU_CLA_SM_CMD_HDR_SKIP;
    }
    else if (cla_raw >> (8U - 3U) == 0b001U) /* ISO 7816-4:2020 p.12 */
    {
        cla.type = SWICC_APDU_CLA_TYPE_RFU;
    }
    else if (cla_raw >> (8U - 4U) ==
                 0b1010U || /* ETSI TS 102 221 V16.4.0 p.76 sec.10.1.1. */
             cla_raw >> (8U - 4U) ==
                 0b1000U /* ETSI TS 102 221 V16.4.0 p.76 sec.10.1.1 and
                            GSM 11.11 4.21.1 pg.32 sec.9.1.*/
    )
    {
        cla.type = SWICC_APDU_CLA_TYPE_PROPRIETARY;
    }
    else
    {
        cla.type = SWICC_APDU_CLA_TYPE_INVALID;
    }
    return cla;
}

swicc_ret_et swicc_apdu_cmd_parse(uint8_t const *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  swicc_apdu_cmd_st *const cmd)
{
    if (buf_raw_len < sizeof(swicc_apdu_cmd_hdr_raw_st) ||
        buf_raw_len > sizeof(swicc_apdu_cmd_hdr_raw_st) + SWICC_DATA_MAX)
    {
        return SWICC_RET_APDU_HDR_TOO_SHORT;
    }

    memset(cmd, 0, sizeof(swicc_apdu_cmd_st));
    cmd->hdr->cla = swicc_apdu_cmd_cla_parse(buf_raw[0]);
    cmd->hdr->ins = buf_raw[1U];
    cmd->hdr->p1 = buf_raw[2U];
    cmd->hdr->p2 = buf_raw[3U];
    cmd->data->len =
        (uint16_t)(buf_raw_len -
                   sizeof(swicc_apdu_cmd_hdr_raw_st)); /* Safe cast due to
                                                         checks at start. */
    memcpy(cmd->data->b, &buf_raw[sizeof(swicc_apdu_cmd_hdr_raw_st)],
           cmd->data->len);
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_apdu_res_deparse(uint8_t *const buf_raw,
                                    uint16_t *const buf_raw_len,
                                    swicc_apdu_cmd_st const *const cmd,
                                    swicc_apdu_res_st const *const res)
{
    /* Ensure there is space in the raw buffers for the whole response. */
    if (*buf_raw_len <
        res->data.len + (res->sw1 == SWICC_APDU_SW1_PROC_NULL ||
                                 res->sw1 == SWICC_APDU_SW1_PROC_ACK_ONE ||
                                 res->sw1 == SWICC_APDU_SW1_PROC_ACK_ALL
                             ? 1U
                             : 2U))
    {
        return SWICC_RET_BUFFER_TOO_SHORT;
    }
    if (res->data.len > SWICC_DATA_MAX)
    {
        /* Unexpected. */
        return SWICC_RET_ERROR;
    }
    *buf_raw_len = res->data.len;
    memcpy(buf_raw, res->data.b, res->data.len);
    uint8_t *const status = &buf_raw[res->data.len];
    uint8_t const sw1_raw = (uint8_t)res->sw1;

    switch (res->sw1)
    {
    case SWICC_APDU_SW1_NORM_NONE:
    case SWICC_APDU_SW1_CHER_P1P2:
    case SWICC_APDU_SW1_CHER_INS:
    case SWICC_APDU_SW1_CHER_CLA:
    case SWICC_APDU_SW1_CHER_UNK:
        if (res->sw2 != 0U)
        {
            return SWICC_RET_APDU_RES_INVALID;
        }
        /* Only the above SW1 bytes need a check that SW2 equals 0. */
        __attribute__((fallthrough));
    case SWICC_APDU_SW1_NORM_BYTES_AVAILABLE:
    case SWICC_APDU_SW1_WARN_NVM_CHGN:
    case SWICC_APDU_SW1_WARN_NVM_CHGM:
    case SWICC_APDU_SW1_EXER_NVM_CHGN:
    case SWICC_APDU_SW1_EXER_NVM_CHGM:
    case SWICC_APDU_SW1_EXER_SEC:
    case SWICC_APDU_SW1_CHER_LEN:
    case SWICC_APDU_SW1_CHER_CLA_FUNC:
    case SWICC_APDU_SW1_CHER_CMD:
    case SWICC_APDU_SW1_CHER_P1P2_INFO:
    case SWICC_APDU_SW1_CHER_LE:
        status[0U] = sw1_raw;
        status[1U] = res->sw2;
        /* Safe cast due to check at the start that ensures res will fit. */
        *buf_raw_len = (uint16_t)(*buf_raw_len + 2U);
        return SWICC_RET_SUCCESS;
    case SWICC_APDU_SW1_PROC_NULL:
        if (res->data.len != 0 || res->sw2 != 0)
        {
            return SWICC_RET_APDU_RES_INVALID;
        }
        buf_raw[0U] = sw1_raw;
        *buf_raw_len = 1U;
        return SWICC_RET_SUCCESS;
    case SWICC_APDU_SW1_PROC_ACK_ALL:
    case SWICC_APDU_SW1_PROC_ACK_ONE:
        /**
         * In this case the response data length indicates the expected data
         * length and not the actual data length that is sent back.
         */
        if (res->sw2 != 0)
        {
            return SWICC_RET_APDU_RES_INVALID;
        }
        /**
         * @note Older ISO 7816-3 standard editions defined two other ways of
         * getting more data that have been deprecated now.
         */
        if (res->sw1 == SWICC_APDU_SW1_PROC_ACK_ONE)
        {
            buf_raw[0U] = cmd->hdr->ins ^ 0xFF;
        }
        else
        {
            buf_raw[0U] = cmd->hdr->ins;
        }
        *buf_raw_len = 1U;
        return SWICC_RET_SUCCESS;
    default:
        /**
         * Treat all other cases as valid status messages that are non-standard.
         */
        status[0U] = sw1_raw;
        status[1U] = res->sw2;
        /* Safe cast due to check at the start that ensures res will fit. */
        *buf_raw_len = (uint16_t)(*buf_raw_len + 2U);
        return SWICC_RET_SUCCESS;
    }
}

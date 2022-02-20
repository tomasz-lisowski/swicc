#include "usim.h"
#include <string.h>

/**
 * @brief Parse the 2 bits indicating secure messaging in a CLA byte based on
 * ISO 7816-4:2020 and ETSI TS 102 221 V16.4.0.
 * @param cla_raw Raw CLA
 * @return 0 OR'd with the right SM enum member based on the CLA.
 */
static apdu_cla_et iso7816_parse_apdu_cmd_sm(uint8_t const cla_raw)
{
    apdu_cla_et cla = 0;
    switch ((cla_raw & 0b00001100) >> 2)
    {
    case 0b00:
        cla |= APDU_CLA_SM_NO;
        break;
    case 0b01:
        cla |= APDU_CLA_SM_PROPRIETARY;
        break;
    case 0b10:
        cla |= APDU_CLA_SM_CMD_HDR_SKIP;
        break;
    case 0b11:
        cla |= APDU_CLA_SM_CMD_HDR_AUTH;
        break;
    }
    return cla;
}

apdu_cla_et usim_parse_apdu_cmd_cla(uint8_t const cla_raw)
{
    apdu_cla_et cla = 0U;
    if (cla_raw >> (8U - 3U) == 0b000U) /* ISO 7816-4:2020 p.13 table.2 */
    {
        const uint8_t lchan = cla_raw & 0b00000011U;
        cla |= lchan;
        cla <<= 28U;

        cla |= APDU_CLA_INTERINDUSTRY;
        cla |= (cla_raw & 0b00010000U) == 0U ? APDU_CLA_CMD_CHAIN_LAST
                                             : APDU_CLA_CMD_CHAIN_MORE;
        cla |= iso7816_parse_apdu_cmd_sm(cla_raw);
    }
    else if (cla_raw >> (8U - 2U) == 0b01U) /* ISO 7816-4:2020 p.13 table.3*/
    {
        const uint8_t lchan = cla_raw & 0b00001111U;
        cla |= lchan + 4U /* ISO 7816-4:2020 p.13 */;
        cla <<= 28U;

        cla |= APDU_CLA_INTERINDUSTRY;
        cla |= (cla_raw & 0b00010000U) == 0U ? APDU_CLA_CMD_CHAIN_LAST
                                             : APDU_CLA_CMD_CHAIN_MORE;
        cla |= (cla_raw & 0b00100000U) == 0U ? APDU_CLA_SM_NO
                                             : APDU_CLA_SM_CMD_HDR_SKIP;
    }
    else if (cla_raw >> (8U - 4U) == 0b1010U ||
             cla_raw >> (8U - 4U) == 0b1000U) /* ETSI TS 102 221 V16.4.0 p.76 */
    {
        const uint8_t lchan = cla_raw & 0b00000011U;
        cla |= lchan;
        cla <<= 28U;

        cla |= APDU_CLA_INTERINDUSTRY;
        cla |=
            APDU_CLA_CMD_CHAIN_LAST; /* ETSI TS 102 221 V16.4.0 p.76 states that
                                        command chaining is not supported */
        cla |= iso7816_parse_apdu_cmd_sm(cla_raw);
    }
    else if (cla_raw >> (8U - 3U) == 0b001U) /* ISO 7816-4:2020 p.12 */
    {
        cla |= APDU_CLA_RFU;
    }
    else
    {
        cla |= APDU_CLA_INVALID;
    }
    return cla;
}

apdu_ins_et usim_parse_apdu_cmd_ins(uint8_t const ins_raw)
{
    /**
     * It is more efficient to check for all the unused instructions than the
     * used ones but without a pre-allocd map of all ops.
     */
    switch (ins_raw)
    {
    case 0x00 ... 0x03:
    case 0x05:
    case 0x07:
    case 0x09 ... 0x0b:
    case 0x0d:
    case 0x11:
    case 0x13:
    case 0x15:
    case 0x18 ... 0x1f:
    case 0x23:
    case 0x27:
    case 0x29:
    case 0x30 ... 0x32:
    case 0x36 ... 0x3f:
    case 0x42 ... 0x43:
    case 0x45:
    case 0x49 ... 0x6f:
    case 0x71 ... 0x81:
    case 0x83:
    case 0x85:
    case 0x89 ... 0x9f:
    case 0xa3:
    case 0xa6 ... 0xaf:
    case 0xb4 ... 0xbf:
    case 0xc1:
    case 0xc4 ... 0xc9:
    case 0xce:
    case 0xd3 ... 0xd5:
    case 0xe3:
    case 0xe5:
    case 0xe7:
    case 0xe9:
    case 0xef ... 0xfd:
    case 0xff:
        return APDU_INS_RFU;
    }
    return ins_raw;
}

usim_ret_et usim_parse_apdu_cmd(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                apdu_cmd_st *const apdu_cmd)
{
    if (buf_raw_len < sizeof(apdu_cmd_hdr_raw_st))
    {
        return USIM_RET_APDU_CMD_TOO_SHORT;
    }

    memset(apdu_cmd, 0, sizeof(apdu_cmd_st));
    apdu_cmd->hdr.class = usim_parse_apdu_cmd_cla(buf_raw[0]);
    apdu_cmd->hdr.instruction = usim_parse_apdu_cmd_ins(buf_raw[1]);
    apdu_cmd->hdr.param_1 = buf_raw[2];
    apdu_cmd->hdr.param_2 = buf_raw[3];
    return USIM_RET_SUCCESS;
}

usim_ret_et usim_deparse_apdu_cmd(uint8_t *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  apdu_cmd_st const *const apdu_cmd)
{
    return USIM_RET_UNKNOWN;
}

usim_ret_et usim_parse_apdu_res(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                apdu_res_st *const apdu_res)
{
    return USIM_RET_UNKNOWN;
}

usim_ret_et usim_deparse_apdu_res(uint8_t *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  apdu_res_st const *const apdu_res)
{
    return USIM_RET_UNKNOWN;
}

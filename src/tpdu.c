#include <string.h>
#include <swicc/swicc.h>

swicc_ret_et swicc_tpdu_cmd_parse(uint8_t const *const buf_raw,
                                  uint16_t const buf_raw_len,
                                  swicc_tpdu_cmd_st *const cmd)
{
    if (buf_raw_len < sizeof(swicc_apdu_cmd_hdr_raw_st) + 1U /* P3 */)
    {
        return SWICC_RET_TPDU_HDR_TOO_SHORT;
    }

    uint16_t const hdr_len = sizeof(swicc_apdu_cmd_hdr_raw_st) + 1U;
    memset(cmd, 0U, sizeof(swicc_tpdu_cmd_st));
    cmd->hdr.cla = swicc_apdu_cmd_cla_parse(buf_raw[0U]);
    cmd->hdr.ins = buf_raw[1U];
    cmd->hdr.p1 = buf_raw[2U];
    cmd->hdr.p2 = buf_raw[3U];
    cmd->p3 = buf_raw[hdr_len - 1U];
    /* Safe cast due to check at start. */
    cmd->data.len = (uint16_t)(buf_raw_len - hdr_len);
    memcpy(cmd->data.b, &buf_raw[hdr_len], cmd->data.len);
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_tpdu_to_apdu(swicc_apdu_cmd_st *const apdu_cmd,
                                swicc_tpdu_cmd_st *const tpdu_cmd)
{
    apdu_cmd->hdr = &tpdu_cmd->hdr;
    apdu_cmd->p3 = &tpdu_cmd->p3;
    apdu_cmd->data = &tpdu_cmd->data;
    return SWICC_RET_SUCCESS;
}

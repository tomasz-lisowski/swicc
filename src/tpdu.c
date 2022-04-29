#include "uicc.h"
#include <string.h>

uicc_ret_et uicc_tpdu_cmd_parse(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                uicc_tpdu_cmd_st *const tpdu_cmd)
{
    if (buf_raw_len < sizeof(uicc_tpdu_cmd_hdr_raw_st))
    {
        return UICC_RET_TPDU_HDR_TOO_SHORT;
    }

    memset(tpdu_cmd, 0, sizeof(uicc_tpdu_cmd_st));
    tpdu_cmd->hdr.hdr_apdu.cla = uicc_apdu_cmd_cla_parse(buf_raw[0]);
    tpdu_cmd->hdr.hdr_apdu.ins = buf_raw[1];
    tpdu_cmd->hdr.hdr_apdu.p1 = buf_raw[2];
    tpdu_cmd->hdr.hdr_apdu.p2 = buf_raw[3];
    tpdu_cmd->hdr.p3 = buf_raw[4];
    return UICC_RET_SUCCESS;
}

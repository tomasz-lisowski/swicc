#include "usim.h"
#include <string.h>

usim_ret_et usim_parse_tpdu_cmd(uint8_t const *const buf_raw,
                                uint16_t const buf_raw_len,
                                tpdu_cmd_st *const tpdu_cmd)
{
    if (buf_raw_len < sizeof(tpdu_cmd_hdr_raw_st))
    {
        return USIM_RET_TPDU_CMD_TOO_SHORT;
    }

    memset(tpdu_cmd, 0, sizeof(tpdu_cmd_st));
    tpdu_cmd->hdr.hdr_apdu.class = usim_parse_apdu_cmd_cla(buf_raw[0]);
    tpdu_cmd->hdr.hdr_apdu.instruction = usim_parse_apdu_cmd_ins(buf_raw[1]);
    tpdu_cmd->hdr.hdr_apdu.param_1 = buf_raw[2];
    tpdu_cmd->hdr.hdr_apdu.param_2 = buf_raw[3];
    tpdu_cmd->hdr.param_3 = buf_raw[4];
    return USIM_RET_SUCCESS;
}

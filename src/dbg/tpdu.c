#include "usim.h"
#include <stdio.h>

usim_ret_et usim_str_tpdu_cmd(char *const buf_str, uint16_t *const buf_str_len,
                              tpdu_cmd_st const *const tpdu_cmd)
{
#ifdef DEBUG
    int bytes_written =
        snprintf(buf_str, *buf_str_len,
                 // clang-format off
                 "(TPDU"
                 "\n  (CLA (CHAIN '%s') (SM '%s') (INFO '%s') (LCHAN %u))"
                 "\n  (INS OP '%s')"
                 "\n  (P1 0x%x)"
                 "\n  (P2 0x%x)"
                 "\n  (P3 0x%x))",
                 // clang-format on
                 usim_dbg_str_apdu_cla_chain(tpdu_cmd->hdr.hdr_apdu.class),
                 usim_dbg_str_apdu_cla_sm(tpdu_cmd->hdr.hdr_apdu.class),
                 usim_dbg_str_apdu_cla_info(tpdu_cmd->hdr.hdr_apdu.class),
                 usim_dbg_str_apdu_cla_lchan(tpdu_cmd->hdr.hdr_apdu.class),
                 usim_dbg_str_apdu_ins(tpdu_cmd->hdr.hdr_apdu.instruction),
                 tpdu_cmd->hdr.hdr_apdu.param_1, tpdu_cmd->hdr.hdr_apdu.param_2,
                 tpdu_cmd->hdr.param_3);
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

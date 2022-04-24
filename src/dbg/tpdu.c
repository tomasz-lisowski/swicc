#include "uicc.h"
#include <stdio.h>

uicc_ret_et uicc_dbg_tpdu_cmd_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_tpdu_cmd_st const *const tpdu_cmd)
{
#ifdef DEBUG
    int bytes_written = snprintf(
        buf_str, *buf_str_len,
        // clang-format off
                 "(TPDU"
                 "\n  (CLA (CHAIN '%s') (SM '%s') (INFO '%s') (LCHAN %u))"
                 "\n  (INS OP '%s')"
                 "\n  (P1 0x%x)"
                 "\n  (P2 0x%x)"
                 "\n  (P3 0x%x))",
        // clang-format on
        uicc_dbg_apdu_cla_ccc_str(tpdu_cmd->hdr.hdr_apdu.class),
        uicc_dbg_apdu_cla_sm_str(tpdu_cmd->hdr.hdr_apdu.class),
        uicc_dbg_apdu_cla_type_str(tpdu_cmd->hdr.hdr_apdu.class),
        tpdu_cmd->hdr.hdr_apdu.class.lchan,
        tpdu_cmd->hdr.hdr_apdu.class.type == UICC_APDU_CLA_TYPE_INTERINDUSTRY
            ? uicc_dbg_apdu_ins_str(tpdu_cmd->hdr.hdr_apdu.instruction)
            : "???",
        tpdu_cmd->hdr.hdr_apdu.param_1, tpdu_cmd->hdr.hdr_apdu.param_2,
        tpdu_cmd->hdr.param_3);
    if (bytes_written < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        *buf_str_len = (uint16_t)
            bytes_written; /* NOTE: Safe cast due to args of snprintf */
        return UICC_RET_SUCCESS;
    }
#else
    return UICC_RET_SUCCESS;
#endif
}

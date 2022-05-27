#include <uicc/uicc.h>
#include <stdio.h>

uicc_ret_et uicc_dbg_tpdu_cmd_str(char *const buf_str,
                                  uint16_t *const buf_str_len,
                                  uicc_tpdu_cmd_st const *const tpdu_cmd)
{
#ifdef DEBUG
    int bytes_written = snprintf(
        buf_str, *buf_str_len,
        // clang-format off
        "(" CLR_KND("TPDU")
        "\n  (" CLR_KND("CLA") " (" CLR_KND("Chain") " " CLR_VAL("'%s'") ") (" CLR_KND("SM") " " CLR_VAL("'%s'") ") (" CLR_KND("Info") " " CLR_VAL("'%s'") ") (" CLR_KND("Logical Channel") " " CLR_VAL("%u") "))"
        "\n  (" CLR_KND("INS") " " CLR_VAL("0x%02X") " = " CLR_VAL("'%s'") ")"
        "\n  (" CLR_KND("P1") " " CLR_VAL("0x%02X") ")"
        "\n  (" CLR_KND("P2") " " CLR_VAL("0x%02X") ")"
        "\n  (" CLR_KND("P3") " " CLR_VAL("0x%02X") "))"
        "\n  (" CLR_KND("Data Len") " " CLR_VAL("%u") "))",
        // clang-format on
        uicc_dbg_apdu_cla_ccc_str(tpdu_cmd->hdr.cla),
        uicc_dbg_apdu_cla_sm_str(tpdu_cmd->hdr.cla),
        uicc_dbg_apdu_cla_type_str(tpdu_cmd->hdr.cla), tpdu_cmd->hdr.cla.lchan,
        tpdu_cmd->hdr.ins,
        tpdu_cmd->hdr.cla.type == UICC_APDU_CLA_TYPE_INTERINDUSTRY
            ? uicc_dbg_apdu_ins_str(tpdu_cmd->hdr.ins)
            : "???",
        tpdu_cmd->hdr.p1, tpdu_cmd->hdr.p2, tpdu_cmd->p3, tpdu_cmd->data.len);
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
    *buf_str_len = 0U;
    return UICC_RET_SUCCESS;
#endif
}

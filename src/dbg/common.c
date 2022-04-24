#include "uicc.h"
#include <stdio.h>

#ifdef DEBUG
static char const *const uicc_dbg_table_str_ret[] = {
    [UICC_RET_UNKNOWN] = "unknown",
    [UICC_RET_SUCCESS] = "success",
    [UICC_RET_APDU_CMD_TOO_SHORT] = "APDU command is too short",
    [UICC_RET_TPDU_CMD_TOO_SHORT] = "TPDU command is too short",
    [UICC_RET_BUFFER_TOO_SHORT] = "provided buffer is too short",

    [UICC_RET_FSM_TRANSITION_WAIT] =
        "wait for state change before running the FSM again",
    [UICC_RET_FSM_TRANSITION_NOW] =
        "run the FSM again without waiting for anything",

    [UICC_RET_PPS_INVALID] = "invalid PPS",
    [UICC_RET_PPS_FAILED] = "PPS is valid but the parameters are not accepted",

    [UICC_RET_ATR_INVALID] = "invalid ATR",
    [UICC_RET_FS_FAILURE] = "unspecified FS crticial error",
    [UICC_RET_FS_FILE_NOT_FOUND] = "file not found",

    [UICC_RET_DO_BERTLV_NOT_FOUND] = "BER-TLV not found",
    [UICC_RET_DO_BERTLV_INVALID] = "BER-TLV invalid",
};
#endif

uicc_ret_et uicc_dbg_ret_str(char *const buf_str, uint16_t *const buf_str_len,
                             uicc_ret_et const ret)
{
#ifdef DEBUG
    int bytes_written = snprintf(buf_str, *buf_str_len,
                                 // clang-format off
                                 "(RET '%s')",
                                 // clang-format on
                                 uicc_dbg_table_str_ret[ret]);
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

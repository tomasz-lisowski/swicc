#include "uicc.h"
#include <stdio.h>

#ifdef DEBUG
static char const *const uicc_dbg_table_str_ret[] = {
    [UICC_RET_UNKNOWN] = "unknown",
    [UICC_RET_SUCCESS] = "success",
    [UICC_RET_ERROR] = "unspecified error",
    [UICC_RET_PARAM_BAD] = "bad parameter",

    [UICC_RET_APDU_HDR_TOO_SHORT] = "APDU header is too short",
    [UICC_RET_APDU_UNHANDLED] = "APDU unhandled",
    [UICC_RET_APDU_RES_INVALID] = "APDU response invalid",
    [UICC_RET_TPDU_HDR_TOO_SHORT] = "TPDU header is too short",
    [UICC_RET_BUFFER_TOO_SHORT] = "provided buffer is too short",

    [UICC_RET_FSM_TRANSITION_WAIT] =
        "wait for state change before running the FSM again",
    [UICC_RET_FSM_TRANSITION_NOW] =
        "run the FSM again without waiting for anything",

    [UICC_RET_PPS_INVALID] = "invalid PPS",
    [UICC_RET_PPS_FAILED] = "PPS is valid but the parameters are not accepted",

    [UICC_RET_ATR_INVALID] = "invalid ATR",
    [UICC_RET_FS_NOT_FOUND] = "not found in FS",

    [UICC_RET_DATO_END] = "DO end of data",
};
#endif

uicc_ret_et uicc_dbg_ret_str(char *const buf_str, uint16_t *const buf_str_len,
                             uicc_ret_et const ret)
{
#ifdef DEBUG
    int bytes_written = snprintf(buf_str, *buf_str_len,
                                 "(" CLR_KND("RET") " " CLR_VAL("'%s'") ")",
                                 uicc_dbg_table_str_ret[ret]);
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
    return UICC_RET_SUCCESS;
#endif
}

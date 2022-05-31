#include <stdio.h>
#include <uicc/uicc.h>

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

    [UICC_RET_PPS_INVALID] = "invalid PPS",
    [UICC_RET_PPS_FAILED] = "PPS is valid but the parameters are not accepted",

    [UICC_RET_ATR_INVALID] = "invalid ATR",
    [UICC_RET_FS_NOT_FOUND] = "not found in FS",

    [UICC_RET_DATO_END] = "DO end of data",
};
#endif

char const *uicc_dbg_ret_str(uicc_ret_et const ret)
{
#ifdef DEBUG
    /**
     * There are not that many return value and none of them will be negative by
     * convention.
     */
    if ((uint32_t)ret <
        sizeof(uicc_dbg_table_str_ret) / sizeof(uicc_dbg_table_str_ret[0U]))
    {
        return uicc_dbg_table_str_ret[ret];
    }
    else
    {
        return "???";
    }
#else
    return NULL;
#endif
}

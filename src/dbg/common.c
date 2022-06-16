#include <stdio.h>
#include <swicc/swicc.h>

#ifdef DEBUG
static char const *const swicc_dbg_table_str_ret[] = {
    [SWICC_RET_UNKNOWN] = "unknown",
    [SWICC_RET_SUCCESS] = "success",
    [SWICC_RET_ERROR] = "error",
    [SWICC_RET_PARAM_BAD] = "bad parameter",

    [SWICC_RET_APDU_HDR_TOO_SHORT] = "APDU header is too short",
    [SWICC_RET_APDU_UNHANDLED] = "APDU unhandled",
    [SWICC_RET_APDU_RES_INVALID] = "APDU response invalid",
    [SWICC_RET_TPDU_HDR_TOO_SHORT] = "TPDU header is too short",
    [SWICC_RET_BUFFER_TOO_SHORT] = "provided buffer is too short",

    [SWICC_RET_PPS_INVALID] = "invalid PPS",
    [SWICC_RET_PPS_FAILED] = "PPS is valid but the parameters are not accepted",

    [SWICC_RET_ATR_INVALID] = "invalid ATR",
    [SWICC_RET_FS_NOT_FOUND] = "not found in FS",

    [SWICC_RET_DATO_END] = "DO end of data",
};
#endif

char const *swicc_dbg_ret_str(swicc_ret_et const ret)
{
#ifdef DEBUG
    /**
     * There are not that many return value and none of them will be negative by
     * convention.
     */
    if ((uint32_t)ret <
        sizeof(swicc_dbg_table_str_ret) / sizeof(swicc_dbg_table_str_ret[0U]))
    {
        return swicc_dbg_table_str_ret[ret];
    }
    else
    {
        return "???";
    }
#else
    return NULL;
#endif
}

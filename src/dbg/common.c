#include "usim.h"
#include <stdio.h>

#ifdef DEBUG
static char const *const usim_dbg_table_str_ret[] = {
    [USIM_RET_UNKNOWN] = "unknown",
    [USIM_RET_SUCCESS] = "success",
    [USIM_RET_APDU_CMD_TOO_SHORT] = "APDU command is too short",
    [USIM_RET_TPDU_CMD_TOO_SHORT] = "TPDU command is too short",
    [USIM_RET_BUFFER_TOO_SHORT] = "provided buffer is too short",
    [USIM_RET_FSM_TRANSITION_WAIT] =
        "wait for state change before running the FSM again",
    [USIM_RET_FSM_TRANSITION_NOW] =
        "run the FSM again without waiting for anything",
};
#endif

usim_ret_et usim_dbg_str_ret(char *const buf_str, uint16_t *const buf_str_len,
                             usim_ret_et ret)
{
#ifdef DEBUG
    int bytes_written = snprintf(buf_str, *buf_str_len,
                                 // clang-format off
                                 "(RET '%s')",
                                 // clang-format on
                                 usim_dbg_table_str_ret[ret]);
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

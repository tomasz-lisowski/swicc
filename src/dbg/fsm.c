#include "usim.h"
#include <stdio.h>

#ifdef DEBUG
static char const *const usim_dbg_table_str_fsm_state[] = {
    [USIM_FSM_STATE_OFF] = "off",
    [USIM_FSM_STATE_ACTIVATION] = "activation",
    [USIM_FSM_STATE_RESET_COLD] = "cold reset",
    [USIM_FSM_STATE_ATR_REQ] = "ATR requested",
    [USIM_FSM_STATE_ATR_RES] = "ATR sent",
    [USIM_FSM_STATE_RESET_WARM] = "warm reset",
    [USIM_FSM_STATE_PPS_REQ] = "PPS request came in",
    [USIM_FSM_STATE_TP_CONFD] = "transmission protocol configured",
};
#endif

usim_ret_et usim_dbg_str_fsm_state(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   usim_fsm_state_et fsm_state)
{
#ifdef DEBUG
    int bytes_written = snprintf(buf_str, *buf_str_len,
                                 // clang-format off
                                 "(FSM"
                                 "\n  (STATE '%s'))",
                                 // clang-format on
                                 usim_dbg_table_str_fsm_state[fsm_state]);
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

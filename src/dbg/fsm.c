#include "uicc.h"
#include <stdio.h>

#ifdef DEBUG
static char const *const uicc_dbg_table_str_fsm_state[] = {
    [UICC_FSM_STATE_OFF] = "off",
    [UICC_FSM_STATE_ACTIVATION] = "activation",
    [UICC_FSM_STATE_RESET_COLD] = "cold reset",
    [UICC_FSM_STATE_ATR_REQ] = "ATR requested",
    [UICC_FSM_STATE_ATR_RES] = "ATR sent",
    [UICC_FSM_STATE_RESET_WARM] = "warm reset",
    [UICC_FSM_STATE_PPS_REQ] = "PPS request came in",
    [UICC_FSM_STATE_CMD_WAIT] = "waiting for command",
    [UICC_FSM_STATE_CMD_DATA] = "waiting for data",
    [UICC_FSM_STATE_CMD_FULL] = "received full/complete message",
};
#endif

uicc_ret_et uicc_dbg_fsm_state_str(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   uicc_fsm_state_et fsm_state)
{
#ifdef DEBUG
    int bytes_written = snprintf(
        buf_str, *buf_str_len,

        "(" CLR_KND("FSM") "\n  (" CLR_KND("State") " " CLR_VAL("'%s'") "))",
        uicc_dbg_table_str_fsm_state[fsm_state]);
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

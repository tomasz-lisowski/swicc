#include <stdio.h>
#include <swicc/swicc.h>

#ifdef DEBUG
static char const *const swicc_dbg_table_str_fsm_state[] = {
    [SWICC_FSM_STATE_OFF] = "off",
    [SWICC_FSM_STATE_ACTIVATION] = "activation",
    [SWICC_FSM_STATE_RESET_COLD] = "cold reset",
    [SWICC_FSM_STATE_ATR_REQ] = "ATR requested",
    [SWICC_FSM_STATE_ATR_RES] = "ATR sent",
    [SWICC_FSM_STATE_RESET_WARM] = "warm reset",
    [SWICC_FSM_STATE_PPS_REQ] = "PPS request coming in",
    [SWICC_FSM_STATE_CMD_WAIT] = "waiting for command",
    [SWICC_FSM_STATE_CMD_PROCEDURE] = "handling APDU and sending procedure",
    [SWICC_FSM_STATE_CMD_DATA] = "waiting for data",
};
#endif

swicc_ret_et swicc_dbg_fsm_state_str(char *const buf_str,
                                     uint16_t *const buf_str_len,
                                     swicc_fsm_state_et const fsm_state)
{
#ifdef DEBUG
    int bytes_written = snprintf(
        buf_str, *buf_str_len,
        // clang-format off
        "(" CLR_KND("FSM")
        "\n  (" CLR_KND("State") " " CLR_VAL("'%s'") "))\n",
        // clang-format on
        swicc_dbg_table_str_fsm_state[fsm_state]);
    if (bytes_written < 0)
    {
        return SWICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        *buf_str_len =
            (uint16_t)bytes_written; /* Safe cast due to args of snprintf */
        return SWICC_RET_SUCCESS;
    }
#else
    *buf_str_len = 0U;
    return SWICC_RET_SUCCESS;
#endif
}

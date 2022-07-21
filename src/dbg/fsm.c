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

char const *swicc_dbg_fsm_state_str(swicc_fsm_state_et const fsm_state)
{
#ifdef DEBUG
    return swicc_dbg_table_str_fsm_state[fsm_state];
#else
    return NULL;
#endif
}

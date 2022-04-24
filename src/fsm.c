#include "uicc.h"
#include <stdbool.h>
#include <string.h>

/**
 * The contact state expected at any point after the cold or warm reset have
 * been completed. It indicates the card is operating normally.
 */
#define FSM_STATE_CONT_READY                                                   \
    (UICC_IO_CONT_RST | UICC_IO_CONT_VCC | UICC_IO_CONT_IO |                   \
     UICC_IO_CONT_CLK | UICC_IO_CONT_VALID_ALL)

static uicc_ret_et fsm_handle_s_off(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx ==
        (UICC_IO_CONT_VCC | UICC_IO_CONT_VALID_ALL))
    {
        uicc_state->internal.fsm_state = UICC_FSM_STATE_ACTIVATION;
    }
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_activation(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx ==
        (UICC_IO_CONT_VCC | UICC_IO_CONT_IO | UICC_IO_CONT_CLK |
         UICC_IO_CONT_VALID_ALL))
    {
        uicc_state->internal.fsm_state = UICC_FSM_STATE_RESET_COLD;
        uicc_state->buf_tx_len = 0;
        return UICC_RET_FSM_TRANSITION_WAIT;
    }
    else if ((uicc_state->cont_state_rx &
              (UICC_IO_CONT_VCC | UICC_IO_CONT_VALID_VCC)) > 0)
    {
        /**
         * Wait for the interface to set the desired state as long as it keeps
         * the VCC on.
         */
        uicc_state->buf_tx_len = 0;
        return UICC_RET_FSM_TRANSITION_WAIT;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_reset_cold(uicc_st *const uicc_state)
{
    /* Request for ATR only occurs when the RST signal goes back to H. */
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /**
         * ISO 7816-3:2006 p.10 sec.6.2.2 states that the card should set I/O to
         * state H within 200 clock cycles (delay t_a).
         */
        uicc_state->cont_state_tx |= UICC_IO_CONT_IO | UICC_IO_CONT_VALID_IO;

        /**
         * TODO: Delay t_f is required here according to ISO 7816-3:2006
         * p.10 sec.6.2.2.
         */
        uicc_state->internal.fsm_state = UICC_FSM_STATE_ATR_REQ;
        uicc_state->buf_tx_len = 0;
        return UICC_RET_FSM_TRANSITION_NOW;
    }
    else if (uicc_state->cont_state_rx ==
             (FSM_STATE_CONT_READY & ~((uint32_t)UICC_IO_CONT_RST)))
    {
        /**
         * RST is still low (L) so the interface needs more time to transition
         * it to high (H).
         */
        uicc_state->buf_tx_len = 0;
        return UICC_RET_FSM_TRANSITION_WAIT;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_atr_req(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        if (uicc_state->buf_tx_len >= UICC_ATR_LEN)
        {
            memcpy(uicc_state->buf_tx, uicc_atr, UICC_ATR_LEN);
            uicc_state->buf_tx_len = UICC_ATR_LEN;
            uicc_state->internal.fsm_state = UICC_FSM_STATE_ATR_RES;
            return UICC_RET_FSM_TRANSITION_WAIT;
        }
        else
        {
            uicc_state->buf_tx_len = UICC_ATR_LEN;
            return UICC_RET_BUFFER_TOO_SHORT;
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_atr_res(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        uicc_state->buf_rx_len > 1)
    {
        /**
         * Here we decide like described in ISO 7816-3:2006 p.11 sec.6.3.1
         */
        if (uicc_state->buf_rx[0] == UICC_PPS_PPSS)
        {
            uicc_state->internal.fsm_state = UICC_FSM_STATE_PPS_REQ;
            uicc_state->buf_tx_len = 0;
            return UICC_RET_FSM_TRANSITION_NOW;
        }
        else
        {
            uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
            uicc_state->buf_tx_len = 0;
            return UICC_RET_FSM_TRANSITION_NOW;
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_reset_warm(uicc_st *const uicc_state)
{
    uicc_ret_et ret = UICC_RET_UNKNOWN;
    ret = uicc_reset(uicc_state);
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
        uicc_state->buf_tx_len = 0;
        return UICC_RET_FSM_TRANSITION_WAIT;
    }
    // TODO
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_pps_req(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        uicc_state->buf_rx_len > 1 && uicc_state->buf_rx[0] == UICC_PPS_PPSS)
    {
        uicc_pps_params_st pps_params = {
            .di_idx = UICC_TP_CONF_DEFAULT,
            .fi_idx = UICC_TP_CONF_DEFAULT,
            .spu = 0,
            .t = 0,
        };
        uicc_ret_et const ret =
            uicc_pps(&pps_params, uicc_state->buf_rx, uicc_state->buf_rx_len,
                     uicc_state->buf_tx, &uicc_state->buf_tx_len);
        if (ret == UICC_RET_SUCCESS)
        {
            /**
             * PPS response has been created and should be sent back then card
             * should wait for a transmission protocol message next.
             */
            uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
            uicc_state->internal.tp.di = uicc_io_di_arr[pps_params.di_idx];
            uicc_state->internal.tp.fi = uicc_io_fi_arr[pps_params.fi_idx];
            uicc_state->internal.tp.fmax = uicc_io_fmax_arr[pps_params.fi_idx];
            uicc_etu(&uicc_state->internal.tp.etu, uicc_state->internal.tp.fi,
                     uicc_state->internal.tp.di, uicc_state->internal.tp.fmax);
            return UICC_RET_FSM_TRANSITION_WAIT;
        }
        else if (UICC_RET_PPS_FAILED)
        {
            /**
             * The interface must send another PPS request and again check if
             * the card accepts them or not, otherwise should disable the card.
             */
            uicc_state->internal.fsm_state = UICC_FSM_STATE_ATR_RES;
            return UICC_RET_FSM_TRANSITION_WAIT;
        }
        else if (ret == UICC_RET_PPS_INVALID)
        {
            /**
             * ISO 7816-3:2006 p.20 sec.9.1 states that if an invalid PPS
             * request comes in, the card should not send anything and just
             * wait.
             */
            uicc_state->buf_tx_len = 0;
            return UICC_RET_FSM_TRANSITION_WAIT;
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_cmd_wait(uicc_st *const uicc_state)
{
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_cmd_hdr(uicc_st *const uicc_state)
{
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_cmd_full(uicc_st *const uicc_state)
{
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et (*const fsm_lookup_handle[])(uicc_st *const) = {
    [UICC_FSM_STATE_OFF] = fsm_handle_s_off,
    [UICC_FSM_STATE_ACTIVATION] = fsm_handle_s_activation,
    [UICC_FSM_STATE_RESET_COLD] = fsm_handle_s_reset_cold,
    [UICC_FSM_STATE_ATR_REQ] = fsm_handle_s_atr_req,
    [UICC_FSM_STATE_ATR_RES] = fsm_handle_s_atr_res,
    [UICC_FSM_STATE_RESET_WARM] = fsm_handle_s_reset_warm,
    [UICC_FSM_STATE_PPS_REQ] = fsm_handle_s_pps_req,
    [UICC_FSM_STATE_CMD_WAIT] = fsm_handle_s_cmd_wait,
    [UICC_FSM_STATE_CMD_HDR] = fsm_handle_s_cmd_hdr,
    [UICC_FSM_STATE_CMD_FULL] = fsm_handle_s_cmd_full,
};

uicc_ret_et uicc_fsm(uicc_st *const uicc_state)
{
    uicc_ret_et const ret =
        fsm_lookup_handle[uicc_state->internal.fsm_state](uicc_state);
    return ret;
}

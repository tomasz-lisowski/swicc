#include "usim.h"
#include <stdbool.h>
#include <string.h>

/**
 * The contact state expected at any point after the cold or warm reset have
 * been completed. It indicates the card is operating normally.
 */
#define FSM_STATE_CONT_READY                                                   \
    (USIM_IO_CONT_RST | USIM_IO_CONT_VCC | USIM_IO_CONT_IO |                   \
     USIM_IO_CONT_CLK | USIM_IO_CONT_VALID_ALL)

static usim_ret_et fsm_handle_s_off(usim_st *const usim_state)
{
    if (usim_state->cont_state_rx ==
        (USIM_IO_CONT_VCC | USIM_IO_CONT_VALID_ALL))
    {
        usim_state->internal.fsm_state = USIM_FSM_STATE_ACTIVATION;
    }
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_activation(usim_st *const usim_state)
{
    if (usim_state->cont_state_rx ==
        (USIM_IO_CONT_VCC | USIM_IO_CONT_IO | USIM_IO_CONT_CLK |
         USIM_IO_CONT_VALID_ALL))
    {
        usim_state->internal.fsm_state = USIM_FSM_STATE_RESET_COLD;
        usim_state->buf_tx_len = 0;
        return USIM_RET_FSM_TRANSITION_WAIT;
    }
    else if ((usim_state->cont_state_rx &
              (USIM_IO_CONT_VCC | USIM_IO_CONT_VALID_VCC)) > 0)
    {
        /**
         * Wait for the interface to set the desired state as long as it keeps
         * the VCC on.
         */
        usim_state->buf_tx_len = 0;
        return USIM_RET_FSM_TRANSITION_WAIT;
    }
    usim_state->internal.fsm_state = USIM_FSM_STATE_OFF;
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_reset_cold(usim_st *const usim_state)
{
    /* Request for ATR only occurs when the RST signal goes back to H. */
    if (usim_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /**
         * ISO 7816-3:2006 p.10 sec.6.2.2 states that the card should set I/O to
         * state H within 200 clock cycles (delay t_a).
         */
        usim_state->cont_state_tx |= USIM_IO_CONT_IO | USIM_IO_CONT_VALID_IO;

        /**
         * TODO: Delay t_f is required here according to ISO 7816-3:2006
         * p.10 sec.6.2.2.
         */
        usim_state->internal.fsm_state = USIM_FSM_STATE_ATR_REQ;
        usim_state->buf_tx_len = 0;
        return USIM_RET_FSM_TRANSITION_NOW;
    }
    else if (usim_state->cont_state_rx ==
             (FSM_STATE_CONT_READY & ~((uint32_t)USIM_IO_CONT_RST)))
    {
        /**
         * RST is still low (L) so the interface needs more time to transition
         * it to high (H).
         */
        usim_state->buf_tx_len = 0;
        return USIM_RET_FSM_TRANSITION_WAIT;
    }
    usim_state->internal.fsm_state = USIM_FSM_STATE_OFF;
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_atr_req(usim_st *const usim_state)
{
    if (usim_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        if (usim_state->buf_tx_len >= USIM_ATR_LEN)
        {
            memcpy(usim_state->buf_tx, usim_atr, USIM_ATR_LEN);
            usim_state->buf_tx_len = USIM_ATR_LEN;
            usim_state->internal.fsm_state = USIM_FSM_STATE_ATR_RES;
            return USIM_RET_FSM_TRANSITION_WAIT;
        }
        else
        {
            usim_state->buf_tx_len = USIM_ATR_LEN;
            return USIM_RET_BUFFER_TOO_SHORT;
        }
    }
    usim_state->internal.fsm_state = USIM_FSM_STATE_OFF;
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_atr_res(usim_st *const usim_state)
{
    if (usim_state->cont_state_rx == FSM_STATE_CONT_READY &&
        usim_state->buf_rx_len > 1)
    {
        /**
         * Here we decide like described in ISO 7816-3:2006 p.11 sec.6.3.1
         */
        if (usim_state->buf_rx[0] == USIM_PPS_PPSS)
        {
            usim_state->internal.fsm_state = USIM_FSM_STATE_PPS_REQ;
            usim_state->buf_tx_len = 0;
            return USIM_RET_FSM_TRANSITION_NOW;
        }
        else
        {
            usim_state->internal.fsm_state = USIM_FSM_STATE_TP_CONFD;
            usim_state->buf_tx_len = 0;
            return USIM_RET_FSM_TRANSITION_NOW;
        }
    }
    usim_state->internal.fsm_state = USIM_FSM_STATE_OFF;
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_reset_warm(usim_st *const usim_state)
{
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_pps_req(usim_st *const usim_state)
{
    if (usim_state->cont_state_rx == FSM_STATE_CONT_READY &&
        usim_state->buf_rx_len > 1 && usim_state->buf_rx[0] == USIM_PPS_PPSS)
    {
        pps_params_st pps_params;
        usim_ret_et const ret =
            usim_pps(&pps_params, usim_state->buf_rx, usim_state->buf_rx_len,
                     usim_state->buf_tx, &usim_state->buf_tx_len);
        if (ret == USIM_RET_SUCCESS)
        {
            /**
             * PPS response has been created and should be sent back then card
             * should wait for a transmission protocol message next.
             */
            usim_state->internal.fsm_state = USIM_FSM_STATE_TP_CONFD;
            return USIM_RET_FSM_TRANSITION_WAIT;
        }
        else if (USIM_RET_PPS_FAILED)
        {
            /**
             * The interface must send another PPS request and again check if
             * the card accepts them or not, otherwise should disable the card.
             */
            usim_state->internal.fsm_state = USIM_FSM_STATE_ATR_RES;
            return USIM_RET_FSM_TRANSITION_WAIT;
        }
        else if (ret == USIM_RET_PPS_INVALID)
        {
            /**
             * ISO 7816-3:2006 p.20 sec.9.1 states that if an invalid PPS
             * request comes in, the card should not send anything and just
             * wait.
             */
            usim_state->buf_tx_len = 0;
            return USIM_RET_FSM_TRANSITION_WAIT;
        }
    }
    usim_state->internal.fsm_state = USIM_FSM_STATE_OFF;
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et fsm_handle_s_tp_confd(usim_st *const usim_state)
{
    usim_state->buf_tx_len = 0;
    return USIM_RET_FSM_TRANSITION_WAIT;
}

static usim_ret_et (*const fsm_lookup_handle[])(usim_st *const) = {
    [USIM_FSM_STATE_OFF] = fsm_handle_s_off,
    [USIM_FSM_STATE_ACTIVATION] = fsm_handle_s_activation,
    [USIM_FSM_STATE_RESET_COLD] = fsm_handle_s_reset_cold,
    [USIM_FSM_STATE_ATR_REQ] = fsm_handle_s_atr_req,
    [USIM_FSM_STATE_ATR_RES] = fsm_handle_s_atr_res,
    [USIM_FSM_STATE_RESET_WARM] = fsm_handle_s_reset_warm,
    [USIM_FSM_STATE_PPS_REQ] = fsm_handle_s_pps_req,
    [USIM_FSM_STATE_TP_CONFD] = fsm_handle_s_tp_confd,
};

usim_ret_et usim_fsm(usim_st *const usim_state)
{
    usim_ret_et const ret =
        fsm_lookup_handle[usim_state->internal.fsm_state](usim_state);
    return ret;
}

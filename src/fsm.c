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
    uicc_ret_et ret;
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
            uicc_state->internal.tp.di = uicc_io_di[pps_params.di_idx];
            uicc_state->internal.tp.fi = uicc_io_fi[pps_params.fi_idx];
            uicc_state->internal.tp.fmax = uicc_io_fmax[pps_params.fi_idx];
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
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        uicc_state->buf_rx_len == 5U /* Expecting just header. */)
    {
        uicc_tpdu_cmd_st tpdu_cmd;
        if (uicc_tpdu_cmd_parse(uicc_state->buf_rx, uicc_state->buf_rx_len,
                                &tpdu_cmd) == UICC_RET_SUCCESS)
        {
            uicc_apdu_cmd_st apdu_cmd;
            if (uicc_tpdu_to_apdu(&apdu_cmd, &tpdu_cmd) == UICC_RET_SUCCESS)
            {
                uicc_apdu_res_st apdu_res;
                uicc_ret_et const apdu_handle_ret =
                    uicc_apduh_demux(uicc_state, &apdu_cmd, &apdu_res);
                if (apdu_handle_ret == UICC_RET_SUCCESS)
                {
                    uicc_ret_et const ret_res = uicc_apdu_res_deparse(
                        uicc_state->buf_tx, &uicc_state->buf_tx_len, &apdu_cmd,
                        &apdu_res);
                    if (ret_res == UICC_RET_APDU_DATA_WAIT)
                    {
                        /**
                         * Store the command header for use in the next state
                         * when the rest of the data is received.
                         */
                        memcpy(&uicc_state->internal.apdu_cur.hdr, apdu_cmd.hdr,
                               sizeof(uicc_state->internal.apdu_cur.hdr));
                        uicc_state->internal.apdu_cur.p3 = *apdu_cmd.p3;

                        /* There is more data to come for this command. */
                        uicc_state->internal.fsm_state =
                            UICC_FSM_STATE_CMD_DATA;
                    }
                    else if (ret_res == UICC_RET_SUCCESS)
                    {
                        /**
                         * The command has been handled from just the header (or
                         * rejected early).
                         */
                        uicc_state->internal.fsm_state =
                            UICC_FSM_STATE_CMD_WAIT;
                    }
                    return UICC_RET_FSM_TRANSITION_WAIT;
                }
            }
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_cmd_data(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        uicc_state->buf_rx_len <= UICC_DATA_MAX)
    {
        uicc_tpdu_cmd_st tpdu_cmd;
        uicc_apdu_cmd_st const apdu_cmd = {
            .hdr = &tpdu_cmd.hdr, .p3 = &tpdu_cmd.p3, .data = &tpdu_cmd.data};

        memcpy(apdu_cmd.hdr, &uicc_state->internal.apdu_cur.hdr,
               sizeof(uicc_state->internal.apdu_cur.hdr));
        *apdu_cmd.p3 = uicc_state->internal.apdu_cur.p3;
        memcpy(apdu_cmd.data->b, uicc_state->buf_rx, uicc_state->buf_rx_len);
        apdu_cmd.data->len = uicc_state->buf_rx_len;

        uicc_apdu_res_st apdu_res;
        uicc_ret_et const apdu_handle_ret =
            uicc_apduh_demux(uicc_state, &apdu_cmd, &apdu_res);
        if (apdu_handle_ret == UICC_RET_SUCCESS)
        {
            if (uicc_apdu_res_deparse(uicc_state->buf_tx,
                                      &uicc_state->buf_tx_len, &apdu_cmd,
                                      &apdu_res) == UICC_RET_SUCCESS)
            {
                uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_FULL;
                return UICC_RET_FSM_TRANSITION_NOW;
            }
        }
    }
    /**
     * @note If the APDU handler wants data multiple times (which is not
     * supported or expected) it leads to a fallthrough to here.
     */
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0;
    return UICC_RET_FSM_TRANSITION_WAIT;
}

static uicc_ret_et fsm_handle_s_cmd_full(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /* Ensure the state of the SIM is ready to handle another command. */
        memset(&uicc_state->internal.apdu_cur, 0,
               sizeof(uicc_state->internal.apdu_cur));
        uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
        return UICC_RET_FSM_TRANSITION_WAIT;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
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
    [UICC_FSM_STATE_CMD_DATA] = fsm_handle_s_cmd_data,
    [UICC_FSM_STATE_CMD_FULL] = fsm_handle_s_cmd_full,
};

uicc_ret_et uicc_fsm(uicc_st *const uicc_state)
{
    uicc_ret_et const ret =
        fsm_lookup_handle[uicc_state->internal.fsm_state](uicc_state);
    return ret;
}

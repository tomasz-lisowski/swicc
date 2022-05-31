#include <stdbool.h>
#include <string.h>
#include <uicc/uicc.h>

/**
 * The contact state expected at any point after the cold or warm reset have
 * been completed. It indicates the card is operating normally.
 */
#define FSM_STATE_CONT_READY                                                   \
    (UICC_IO_CONT_RST | UICC_IO_CONT_VCC | UICC_IO_CONT_IO |                   \
     UICC_IO_CONT_CLK | UICC_IO_CONT_VALID_ALL)

static uicc_fsmh_ft fsm_handle_s_off;
static void fsm_handle_s_off(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx ==
        (UICC_IO_CONT_VCC | UICC_IO_CONT_VALID_ALL))
    {
        uicc_state->internal.fsm_state = UICC_FSM_STATE_ACTIVATION;
    }
    uicc_state->buf_rx_len = 0U;
    uicc_state->buf_tx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_activation;
static void fsm_handle_s_activation(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx ==
        (UICC_IO_CONT_VCC | UICC_IO_CONT_IO | UICC_IO_CONT_CLK |
         UICC_IO_CONT_VALID_ALL))
    {
        uicc_state->internal.fsm_state = UICC_FSM_STATE_RESET_COLD;
        uicc_state->buf_rx_len = 0U;
        uicc_state->buf_tx_len = 0U;
        return;
    }
    else if ((uicc_state->cont_state_rx &
              (UICC_IO_CONT_VCC | UICC_IO_CONT_VALID_VCC)) > 0)
    {
        /**
         * Wait for the interface to set the desired state as long as it keeps
         * the VCC on.
         */
        uicc_state->buf_rx_len = 0U;
        uicc_state->buf_tx_len = 0U;
        return;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_rx_len = 0U;
    uicc_state->buf_tx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_reset_cold;
static void fsm_handle_s_reset_cold(uicc_st *const uicc_state)
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
        uicc_state->buf_rx_len = 0U;
        uicc_state->buf_tx_len = 0U;
        return;
    }
    else if (uicc_state->cont_state_rx ==
             (FSM_STATE_CONT_READY & ~((uint32_t)UICC_IO_CONT_RST)))
    {
        /**
         * RST is still low (L) so the interface needs more time to transition
         * it to high (H).
         */
        uicc_state->buf_rx_len = 0U;
        uicc_state->buf_tx_len = 0U;
        return;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_rx_len = 0U;
    uicc_state->buf_tx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_atr_res;
static void fsm_handle_s_atr_req(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        if (uicc_state->buf_tx_len >= UICC_ATR_LEN)
        {
            memcpy(uicc_state->buf_tx, uicc_atr, UICC_ATR_LEN);
            /* Get first byte of header (PPS or APDU). */
            uicc_state->buf_rx_len = 1U;
            uicc_state->buf_tx_len = UICC_ATR_LEN;
            uicc_state->internal.fsm_state = UICC_FSM_STATE_ATR_RES;
            return;
        }
        /* TX buffer is too short. */
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_rx_len = 0U;
    uicc_state->buf_tx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_atr_res;
static void fsm_handle_s_atr_res(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        uicc_state->buf_rx_len == 1U)
    {
        /**
         * Here we decide like described in ISO 7816-3:2006 p.11 sec.6.3.1
         */
        if (uicc_state->buf_rx[0U] == UICC_PPS_PPSS)
        {
            /**
             * Clear internally held PPS. This will tell the PPS REQ state that
             * the new data is part of a new PPS.
             */
            memcpy(uicc_state->internal.pps, uicc_state->buf_rx,
                   uicc_state->buf_rx_len);
            /* Safe cast since RX len is 1 here. */
            uicc_state->internal.pps_len = (uint8_t)uicc_state->buf_rx_len;

            uicc_state->internal.fsm_state = UICC_FSM_STATE_PPS_REQ;
            uicc_state->buf_rx_len = 0U;
            uicc_state->buf_tx_len = 0U;
            return;
        }
        else
        {
            memcpy(&uicc_state->internal.tpdu_hdr, uicc_state->buf_rx,
                   uicc_state->buf_rx_len);
            uicc_state->internal.tpdu_hdr_len = 1U;
            uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
            uicc_state->buf_rx_len = 0U;
            uicc_state->buf_tx_len = 0U;
            return;
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_rx_len = 0U;
    uicc_state->buf_tx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_reset_warm;
static void fsm_handle_s_reset_warm(uicc_st *const uicc_state)
{
    uicc_ret_et ret;
    ret = uicc_reset(uicc_state);
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
        uicc_state->buf_rx_len = 0U;
        uicc_state->buf_tx_len = 0U;
        return;
    }
    /**
     * @todo Implement warm reset
     */
    uicc_state->buf_rx_len = 0U;
    uicc_state->buf_tx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_pps_req;
static void fsm_handle_s_pps_req(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        uicc_state->internal.pps_len + uicc_state->buf_rx_len <=
            UICC_PPS_LEN_MAX)
    {
        /* Copy the new PPS bytess into the internally held PPS buffer. */
        memcpy(&uicc_state->internal.pps[uicc_state->internal.pps_len],
               uicc_state->buf_rx, uicc_state->buf_rx_len);
        /* Safe since it was checked in the first 'if'. */
        uicc_state->internal.pps_len =
            (uint8_t)(uicc_state->internal.pps_len + uicc_state->buf_rx_len);

        /**
         * Shortest PPS is just PPSS + PPS0 + PCK. We will know the full length
         * of the PPS when PPS0 is received hence the 2 (PPSS + PPS0).
         */
        if (uicc_state->internal.pps_len < 2U)
        {
            /* Get as many bytes as possible until (and including) PPS0. */
            /* Safe cast due to the 'if' checking PPS len is less than 2. */
            uicc_state->buf_rx_len =
                (uint16_t)(2U - uicc_state->internal.pps_len);
            uicc_state->buf_tx_len = 0U;
            return;
        }
        else
        {
            uint8_t pps_len_exp;
            if (uicc_pps_len(uicc_state->internal.pps,
                             uicc_state->internal.pps_len,
                             &pps_len_exp) == UICC_RET_SUCCESS)
            {
                if (uicc_state->internal.pps_len == pps_len_exp)
                {
                    /**
                     * Can proceed to handling the PPS as-is sicne it was all
                     * received.
                     */
                }
                else if (uicc_state->internal.pps_len < pps_len_exp)
                {
                    /**
                     * Did not receive the full PPS yet so have to get the
                     * remaining PPS bytes.
                     */
                    /**
                     * Safe cast due to the check that PPS length is less than
                     * PPS expected length.
                     */
                    uicc_state->buf_rx_len =
                        (uint16_t)(pps_len_exp - uicc_state->internal.pps_len);
                    uicc_state->buf_tx_len = 0U;
                    return;
                }
                else
                {
                    /* This condition was checked in the first 'if'. */
                    __builtin_unreachable();
                }

                uicc_pps_params_st pps_params = {
                    .di_idx = UICC_TP_CONF_DEFAULT,
                    .fi_idx = UICC_TP_CONF_DEFAULT,
                    .spu = 0U,
                    .t = 0U,
                };
                uicc_ret_et const ret =
                    uicc_pps(&pps_params, uicc_state->internal.pps,
                             uicc_state->internal.pps_len, uicc_state->buf_tx,
                             &uicc_state->buf_tx_len);
                if (ret == UICC_RET_SUCCESS)
                {
                    /**
                     * PPS response has been created and should be sent back
                     * then card should wait for a transmission protocol message
                     * next.
                     */
                    uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
                    uicc_state->internal.tp.di = uicc_io_di[pps_params.di_idx];
                    uicc_state->internal.tp.fi = uicc_io_fi[pps_params.fi_idx];
                    uicc_state->internal.tp.fmax =
                        uicc_io_fmax[pps_params.fi_idx];
                    uicc_etu(&uicc_state->internal.tp.etu,
                             uicc_state->internal.tp.fi,
                             uicc_state->internal.tp.di,
                             uicc_state->internal.tp.fmax);
                    uicc_state->internal.tpdu_processed = false;
                    uicc_state->buf_rx_len = 0U;
                    return;
                }
                else if (ret == UICC_RET_PPS_FAILED)
                {
                    /* PPS failed so wait for another PPS to come in. */
                    uicc_state->internal.fsm_state = UICC_FSM_STATE_ATR_RES;
                    uicc_state->buf_rx_len =
                        1U; /* Expecting another PPS so read its CLA (=0xFF). */
                    return;
                }
                else if (ret == UICC_RET_PPS_INVALID)
                {
                    /**
                     * ISO 7816-3:2006 p.20 sec.9.1 states that if an invalid
                     * PPS request comes in, the card should not send anything
                     * and just wait.
                     */
                    uicc_state->internal.fsm_state = UICC_FSM_STATE_ATR_RES;
                    uicc_state->buf_rx_len =
                        1U; /* Expecting another PPS so read its CLA (=0xFF). */
                    uicc_state->buf_tx_len =
                        0U; /* There is no response for an invalid PPS. */
                    return;
                }
            }
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0U;
    uicc_state->buf_rx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_cmd_wait;
static void fsm_handle_s_cmd_wait(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /* Reset any state left-over from handling the previous APDU. */
        if (uicc_state->internal.tpdu_processed == true)
        {
            memset(&uicc_state->internal.tpdu_cur, 0U,
                   sizeof(uicc_state->internal.tpdu_cur));
            memset(uicc_state->internal.tpdu_hdr, 0U,
                   sizeof(uicc_state->internal.tpdu_hdr));
            uicc_state->internal.tpdu_hdr_len = 0U;
            uicc_state->internal.procedure_count = 0U;
            uicc_state->internal.tpdu_processed = false;
        }

        /* Safe cast since uint16 + uint8 will never overflow a uint32. */
        uint32_t const hdr_len = (uint32_t)(uicc_state->buf_rx_len +
                                            uicc_state->internal.tpdu_hdr_len);
        if (hdr_len <= 5U)
        {
            /**
             * Append new header bytes to the internally kept (temporary)
             * header.
             */
            memcpy(&uicc_state->internal
                        .tpdu_hdr[uicc_state->internal.tpdu_hdr_len],
                   uicc_state->buf_rx, uicc_state->buf_rx_len);
            /* Safe cast due to check of header length. */
            uicc_state->internal.tpdu_hdr_len = (uint8_t)hdr_len;

            /**
             * Check if received full header, if not, get the remaining bytes,
             * if yes, parse the header and use the parser output to decide what
             * to do next.
             */
            if (uicc_state->internal.tpdu_hdr_len == 5U)
            {
                /**
                 * Received the complete header and it has not been processed
                 * yet so we process it here.
                 */
                if (uicc_tpdu_cmd_parse(uicc_state->internal.tpdu_hdr,
                                        uicc_state->internal.tpdu_hdr_len,
                                        &uicc_state->internal.tpdu_cur) ==
                    UICC_RET_SUCCESS)
                {
                    if (uicc_tpdu_to_apdu(&uicc_state->internal.apdu_cur,
                                          &uicc_state->internal.tpdu_cur) ==
                        UICC_RET_SUCCESS)
                    {
                        uicc_state->internal.fsm_state =
                            UICC_FSM_STATE_CMD_PROCEDURE;
                        uicc_state->buf_rx_len =
                            0U; /* Don't get more data while transitioning. */
                        uicc_state->buf_tx_len = 0U;
                        return;
                    }
                }
            }
            else if (hdr_len < 5U)
            {
                /* Get the remainder of the header. */
                uicc_state->buf_tx_len = 0U;
                /* Safe cast since header length is less than 5 here. */
                uicc_state->buf_rx_len = (uint16_t)(5U - hdr_len);
                return;
            }
            else
            {
                /* Header can't have more than 5 bytes... */
                __builtin_unreachable();
            }
        }

        /**
         * Contact state is still fine so just return to the same state but make
         * sure the header is cleared when new header is received.
         */
        uicc_state->internal.tpdu_processed = true;
        uicc_state->buf_tx_len = 0U;
        uicc_state->buf_rx_len = 5U; /* Receive a new header. */
        return;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0U;
    uicc_state->buf_rx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_cmd_procedure;
static void fsm_handle_s_cmd_procedure(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        uicc_apdu_res_st apdu_res;
        uicc_ret_et const apdu_handle_ret =
            uicc_apduh_demux(uicc_state, &uicc_state->internal.apdu_cur,
                             &apdu_res, uicc_state->internal.procedure_count);
        if (apdu_handle_ret == UICC_RET_SUCCESS)
        {
            uicc_ret_et const ret_res = uicc_apdu_res_deparse(
                uicc_state->buf_tx, &uicc_state->buf_tx_len,
                &uicc_state->internal.apdu_cur, &apdu_res);
            if (ret_res == UICC_RET_SUCCESS)
            {
                if (apdu_res.sw1 == UICC_APDU_SW1_PROC_ACK_ONE ||
                    apdu_res.sw1 == UICC_APDU_SW1_PROC_ACK_ALL)
                {
                    if (uicc_state->internal.procedure_count + 1 <=
                        sizeof(uint32_t))
                    {
                        /* Sending an ACK procedure byte. */
                        uicc_state->internal.procedure_count += 1U;

                        /**
                         * There is more data to come for this command.
                         */
                        uicc_state->internal.fsm_state =
                            UICC_FSM_STATE_CMD_DATA;

                        if (apdu_res.data.len == 0U)
                        {
                            /* ACK is sent but no data is expected. */
                            uicc_state->buf_rx_len = 0U;
                            return;
                        }
                        else
                        {
                            uicc_state->buf_rx_len = apdu_res.data.len;
                            return;
                        }
                    }
                }
                else
                {
                    /* Command has been handled. */
                    uicc_state->internal.tpdu_processed = true;
                    uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
                    uicc_state->buf_rx_len = 5U; /* Receive a new header. */
                    return;
                }
            }
            /**
             * Contact state is still fine so just return to waiting for
             * command.
             */
            uicc_state->internal.tpdu_processed = true;
            uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
            uicc_state->buf_tx_len = 0U;
            uicc_state->buf_rx_len = 5U; /* Receive a new header. */
            return;
        }
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0U;
    uicc_state->buf_rx_len = 0U;
    return;
}

static uicc_fsmh_ft fsm_handle_s_cmd_data;
static void fsm_handle_s_cmd_data(uicc_st *const uicc_state)
{
    if (uicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /**
         * Make sure the data will fit in the data buffer i.e. if it will fit in
         * one APDU.
         */
        if (uicc_state->internal.apdu_cur.data->len + uicc_state->buf_rx_len <=
            UICC_DATA_MAX)
        {
            /* Get the data. */
            memcpy(&uicc_state->internal.apdu_cur.data
                        ->b[uicc_state->internal.apdu_cur.data->len],
                   uicc_state->buf_rx, uicc_state->buf_rx_len);
            /**
             * Safe cast due to 'if' condition that checks for data max
             * overflow.
             */
            uicc_state->internal.apdu_cur.data->len =
                (uint16_t)(uicc_state->internal.apdu_cur.data->len +
                           uicc_state->buf_rx_len);

            /* After receiving data, give back a procedure. */
            uicc_state->internal.tpdu_processed = true;
            uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_PROCEDURE;
            uicc_state->buf_tx_len = 0U;

            /**
             * No data is expected between receiving the command data and
             * sending a procedure.
             */
            uicc_state->buf_rx_len = 0U;
            return;
        }

        /* Contact state is still fine so just return to waiting for command. */
        uicc_state->internal.tpdu_processed = true;
        uicc_state->buf_tx_len = 0U;
        uicc_state->buf_rx_len = 5U; /* Receive a new header. */
        uicc_state->internal.fsm_state = UICC_FSM_STATE_CMD_WAIT;
        return;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->buf_tx_len = 0U;
    uicc_state->buf_rx_len = 0U;
    return;
}

static uicc_fsmh_ft *const uicc_fsmh[] = {
    [UICC_FSM_STATE_OFF] = fsm_handle_s_off,
    [UICC_FSM_STATE_ACTIVATION] = fsm_handle_s_activation,
    [UICC_FSM_STATE_RESET_COLD] = fsm_handle_s_reset_cold,
    [UICC_FSM_STATE_ATR_REQ] = fsm_handle_s_atr_req,
    [UICC_FSM_STATE_ATR_RES] = fsm_handle_s_atr_res,
    [UICC_FSM_STATE_RESET_WARM] = fsm_handle_s_reset_warm,
    [UICC_FSM_STATE_PPS_REQ] = fsm_handle_s_pps_req,
    [UICC_FSM_STATE_CMD_WAIT] = fsm_handle_s_cmd_wait,
    [UICC_FSM_STATE_CMD_PROCEDURE] = fsm_handle_s_cmd_procedure,
    [UICC_FSM_STATE_CMD_DATA] = fsm_handle_s_cmd_data,
};

void uicc_fsm(uicc_st *const uicc_state)
{
    uicc_fsmh[uicc_state->internal.fsm_state](uicc_state);
}

#include <stdbool.h>
#include <string.h>
#include <swicc/swicc.h>

static swicc_fsmh_ft fsm_handle_s_off;
static void fsm_handle_s_off(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx ==
        (SWICC_IO_CONT_VCC | SWICC_IO_CONT_VALID_ALL))
    {
        swicc_state->internal.fsm_state = SWICC_FSM_STATE_ACTIVATION;
    }
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_activation;
static void fsm_handle_s_activation(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx ==
        (SWICC_IO_CONT_VCC | SWICC_IO_CONT_IO | SWICC_IO_CONT_CLK |
         SWICC_IO_CONT_VALID_ALL))
    {
        swicc_state->internal.fsm_state = SWICC_FSM_STATE_RESET_COLD;
        swicc_state->buf_rx_len = 0U;
        swicc_state->buf_tx_len = 0U;
        return;
    }
    else if ((swicc_state->cont_state_rx &
              (SWICC_IO_CONT_VCC | SWICC_IO_CONT_VALID_VCC)) > 0)
    {
        /**
         * Wait for the interface to set the desired state as long as it keeps
         * the VCC on.
         */
        swicc_state->buf_rx_len = 0U;
        swicc_state->buf_tx_len = 0U;
        return;
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_reset_cold;
static void fsm_handle_s_reset_cold(swicc_st *const swicc_state)
{
    /* Request for ATR only occurs when the RST signal goes back to H. */
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /**
         * ISO/IEC 7816-3:2006 clause.6.2.2 states that the card should set
         * I/O to state H within 200 clock cycles (delay t_a).
         */
        swicc_state->cont_state_tx |= SWICC_IO_CONT_IO | SWICC_IO_CONT_VALID_IO;

        /**
         * @todo: Delay t_f is required here according to ISO/IEC 7816-3:2006
         * clause.6.2.2.
         */
        swicc_state->internal.fsm_state = SWICC_FSM_STATE_ATR_REQ;
        swicc_state->buf_rx_len = 0U;
        swicc_state->buf_tx_len = 0U;
        return;
    }
    else if (swicc_state->cont_state_rx ==
             (FSM_STATE_CONT_READY & ~((uint32_t)SWICC_IO_CONT_RST)))
    {
        /**
         * RST is still low (L) so the interface needs more time to transition
         * it to high (H).
         */
        swicc_state->buf_rx_len = 0U;
        swicc_state->buf_tx_len = 0U;
        return;
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_atr_res;
static void fsm_handle_s_atr_req(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        if (swicc_state->buf_tx_len >= SWICC_ATR_LEN)
        {
            memcpy(swicc_state->buf_tx, swicc_atr, SWICC_ATR_LEN);
            /* Get first byte of header (PPS or APDU). */
            swicc_state->buf_rx_len = 1U;
            swicc_state->buf_tx_len = SWICC_ATR_LEN;
            swicc_state->internal.fsm_state = SWICC_FSM_STATE_ATR_RES;
            return;
        }
        /* TX buffer is too short. */
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_atr_res;
static void fsm_handle_s_atr_res(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        swicc_state->buf_rx_len == 1U)
    {
        /**
         * Here we decide like described in ISO/IEC 7816-3:2006
         * clause.6.3.1
         */
        if (swicc_state->buf_rx[0U] == SWICC_PPS_PPSS)
        {
            /**
             * Clear internally held PPS. This will tell the PPS REQ state that
             * the new data is part of a new PPS.
             */
            memcpy(swicc_state->internal.pps, swicc_state->buf_rx,
                   swicc_state->buf_rx_len);
            /* Safe cast since RX len is 1 here. */
            swicc_state->internal.pps_len = (uint8_t)swicc_state->buf_rx_len;

            swicc_state->internal.fsm_state = SWICC_FSM_STATE_PPS_REQ;
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = 0U;
            return;
        }
        else
        {
            memcpy(&swicc_state->internal.tpdu_hdr, swicc_state->buf_rx,
                   swicc_state->buf_rx_len);
            swicc_state->internal.tpdu_hdr_len = 1U;
            swicc_state->internal.fsm_state = SWICC_FSM_STATE_CMD_WAIT;
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = 0U;
            return;
        }
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_reset_warm;
static void fsm_handle_s_reset_warm(swicc_st *const swicc_state)
{
    swicc_ret_et ret;
    ret = swicc_reset(swicc_state);
    if (ret != SWICC_RET_SUCCESS)
    {
        swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
        swicc_state->buf_rx_len = 0U;
        swicc_state->buf_tx_len = 0U;
        return;
    }
    /**
     * @todo Implement warm reset
     */
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_pps_req;
static void fsm_handle_s_pps_req(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY &&
        swicc_state->internal.pps_len + swicc_state->buf_rx_len <=
            SWICC_PPS_LEN_MAX)
    {
        /* Copy the new PPS bytess into the internally held PPS buffer. */
        memcpy(&swicc_state->internal.pps[swicc_state->internal.pps_len],
               swicc_state->buf_rx, swicc_state->buf_rx_len);
        /* Safe since it was checked in the first 'if'. */
        swicc_state->internal.pps_len =
            (uint8_t)(swicc_state->internal.pps_len + swicc_state->buf_rx_len);

        /**
         * Shortest PPS is just PPSS + PPS0 + PCK. We will know the full length
         * of the PPS when PPS0 is received hence the 2 (PPSS + PPS0).
         */
        if (swicc_state->internal.pps_len < 2U)
        {
            /* Get as many bytes as possible until (and including) PPS0. */
            /* Safe cast due to the 'if' checking PPS len is less than 2. */
            swicc_state->buf_rx_len =
                (uint16_t)(2U - swicc_state->internal.pps_len);
            swicc_state->buf_tx_len = 0U;
            return;
        }
        else
        {
            uint8_t pps_len_exp;
            if (swicc_pps_len(swicc_state->internal.pps,
                              swicc_state->internal.pps_len,
                              &pps_len_exp) == SWICC_RET_SUCCESS)
            {
                if (swicc_state->internal.pps_len == pps_len_exp)
                {
                    /**
                     * Can proceed to handling the PPS as-is sicne it was all
                     * received.
                     */
                }
                else if (swicc_state->internal.pps_len < pps_len_exp)
                {
                    /**
                     * Did not receive the full PPS yet so have to get the
                     * remaining PPS bytes.
                     */
                    /**
                     * Safe cast due to the check that PPS length is less than
                     * PPS expected length.
                     */
                    swicc_state->buf_rx_len =
                        (uint16_t)(pps_len_exp - swicc_state->internal.pps_len);
                    swicc_state->buf_tx_len = 0U;
                    return;
                }
                else
                {
                    /* This condition was checked in the first 'if'. */
                    __builtin_unreachable();
                }

                swicc_pps_params_st pps_params = {
                    .di_idx = SWICC_TP_CONF_DEFAULT,
                    .fi_idx = SWICC_TP_CONF_DEFAULT,
                    .spu = 0U,
                    .t = 0U,
                };
                swicc_ret_et const ret =
                    swicc_pps(&pps_params, swicc_state->internal.pps,
                              swicc_state->internal.pps_len,
                              swicc_state->buf_tx, &swicc_state->buf_tx_len);
                if (ret == SWICC_RET_SUCCESS)
                {
                    /**
                     * PPS response has been created and should be sent back
                     * then card should wait for a transmission protocol message
                     * next.
                     */
                    swicc_state->internal.fsm_state = SWICC_FSM_STATE_CMD_WAIT;
                    swicc_state->internal.tp.di =
                        swicc_io_di[pps_params.di_idx];
                    swicc_state->internal.tp.fi =
                        swicc_io_fi[pps_params.fi_idx];
                    swicc_state->internal.tp.fmax =
                        swicc_io_fmax[pps_params.fi_idx];
                    swicc_etu(&swicc_state->internal.tp.etu,
                              swicc_state->internal.tp.fi,
                              swicc_state->internal.tp.di,
                              swicc_state->internal.tp.fmax);
                    swicc_state->internal.tpdu_processed = false;
                    swicc_state->buf_rx_len = 0U;
                    return;
                }
                else if (ret == SWICC_RET_PPS_FAILED)
                {
                    /* PPS failed so wait for another PPS to come in. */
                    swicc_state->internal.fsm_state = SWICC_FSM_STATE_ATR_RES;
                    swicc_state->buf_rx_len =
                        1U; /* Expecting another PPS so read its CLA (=0xFF). */
                    return;
                }
                else if (ret == SWICC_RET_PPS_INVALID)
                {
                    /**
                     * ISO/IEC 7816-3:2006 clause.9.1 states that if an
                     * invalid PPS request comes in, the card should not send
                     * anything and just wait.
                     */
                    swicc_state->internal.fsm_state = SWICC_FSM_STATE_ATR_RES;
                    swicc_state->buf_rx_len =
                        1U; /* Expecting another PPS so read its CLA (=0xFF). */
                    swicc_state->buf_tx_len =
                        0U; /* There is no response for an invalid PPS. */
                    return;
                }
            }
        }
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_tx_len = 0U;
    swicc_state->buf_rx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_cmd_wait;
static void fsm_handle_s_cmd_wait(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /* Reset any state left-over from handling the previous APDU. */
        if (swicc_state->internal.tpdu_processed == true)
        {
            memset(&swicc_state->internal.tpdu_cur, 0U,
                   sizeof(swicc_state->internal.tpdu_cur));
            memset(swicc_state->internal.tpdu_hdr, 0U,
                   sizeof(swicc_state->internal.tpdu_hdr));
            swicc_state->internal.tpdu_hdr_len = 0U;
            swicc_state->internal.procedure_count = 0U;
            swicc_state->internal.tpdu_processed = false;
        }

        /* Safe cast since uint16 + uint8 will never overflow a uint32. */
        uint32_t const hdr_len = (uint32_t)(swicc_state->buf_rx_len +
                                            swicc_state->internal.tpdu_hdr_len);
        if (hdr_len <= 5U)
        {
            /**
             * Append new header bytes to the internally kept (temporary)
             * header.
             */
            memcpy(&swicc_state->internal
                        .tpdu_hdr[swicc_state->internal.tpdu_hdr_len],
                   swicc_state->buf_rx, swicc_state->buf_rx_len);
            /* Safe cast due to check of header length. */
            swicc_state->internal.tpdu_hdr_len = (uint8_t)hdr_len;

            /**
             * Check if received full header, if not, get the remaining bytes,
             * if yes, parse the header and use the parser output to decide what
             * to do next.
             */
            if (swicc_state->internal.tpdu_hdr_len == 5U)
            {
                /**
                 * Received the complete header and it has not been processed
                 * yet so we process it here.
                 */
                if (swicc_tpdu_cmd_parse(swicc_state->internal.tpdu_hdr,
                                         swicc_state->internal.tpdu_hdr_len,
                                         &swicc_state->internal.tpdu_cur) ==
                    SWICC_RET_SUCCESS)
                {
                    if (swicc_tpdu_to_apdu(&swicc_state->internal.apdu_cur,
                                           &swicc_state->internal.tpdu_cur) ==
                        SWICC_RET_SUCCESS)
                    {
                        swicc_state->internal.fsm_state =
                            SWICC_FSM_STATE_CMD_PROCEDURE;
                        swicc_state->buf_rx_len =
                            0U; /* Don't get more data while transitioning. */
                        swicc_state->buf_tx_len = 0U;
                        return;
                    }
                }
            }
            else if (hdr_len < 5U)
            {
                /* Get the remainder of the header. */
                swicc_state->buf_tx_len = 0U;
                /* Safe cast since header length is less than 5 here. */
                swicc_state->buf_rx_len = (uint16_t)(5U - hdr_len);
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
        swicc_state->internal.tpdu_processed = true;
        swicc_state->buf_tx_len = 0U;
        swicc_state->buf_rx_len = 5U; /* Receive a new header. */
        return;
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_tx_len = 0U;
    swicc_state->buf_rx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_cmd_procedure;
static void fsm_handle_s_cmd_procedure(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        swicc_apdu_res_st apdu_res;
        swicc_ret_et const apdu_handle_ret =
            swicc_apduh_demux(swicc_state, &swicc_state->internal.apdu_cur,
                              &apdu_res, swicc_state->internal.procedure_count);
        if (apdu_handle_ret == SWICC_RET_SUCCESS)
        {
            swicc_ret_et const ret_res = swicc_apdu_res_deparse(
                swicc_state->buf_tx, &swicc_state->buf_tx_len,
                &swicc_state->internal.apdu_cur, &apdu_res);
            if (ret_res == SWICC_RET_SUCCESS)
            {
                if (apdu_res.sw1 == SWICC_APDU_SW1_PROC_ACK_ONE ||
                    apdu_res.sw1 == SWICC_APDU_SW1_PROC_ACK_ALL)
                {
                    if (swicc_state->internal.procedure_count + 1 <=
                        sizeof(uint32_t))
                    {
                        /* Sending an ACK procedure byte. */
                        swicc_state->internal.procedure_count += 1U;

                        /**
                         * There is more data to come for this command.
                         */
                        swicc_state->internal.fsm_state =
                            SWICC_FSM_STATE_CMD_DATA;

                        if (apdu_res.data.len == 0U)
                        {
                            /* ACK is sent but no data is expected. */
                            swicc_state->buf_rx_len = 0U;
                            return;
                        }
                        else
                        {
                            swicc_state->buf_rx_len = apdu_res.data.len;
                            return;
                        }
                    }
                }
                else
                {
                    /* Command has been handled. */
                    swicc_state->internal.tpdu_processed = true;
                    swicc_state->internal.fsm_state = SWICC_FSM_STATE_CMD_WAIT;
                    swicc_state->buf_rx_len = 5U; /* Receive a new header. */
                    return;
                }
            }
        }
        /**
         * Contact state is still fine so just return to waiting for command.
         */
        swicc_state->internal.tpdu_processed = true;
        swicc_state->internal.fsm_state = SWICC_FSM_STATE_CMD_WAIT;
        swicc_state->buf_tx_len = 0U;
        swicc_state->buf_rx_len = 5U; /* Receive a new header. */
        return;
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_tx_len = 0U;
    swicc_state->buf_rx_len = 0U;
    return;
}

static swicc_fsmh_ft fsm_handle_s_cmd_data;
static void fsm_handle_s_cmd_data(swicc_st *const swicc_state)
{
    if (swicc_state->cont_state_rx == FSM_STATE_CONT_READY)
    {
        /**
         * Make sure the data will fit in the data buffer i.e. if it will fit in
         * one APDU.
         */
        if (swicc_state->internal.apdu_cur.data->len +
                swicc_state->buf_rx_len <=
            SWICC_DATA_MAX)
        {
            /* Get the data. */
            memcpy(&swicc_state->internal.apdu_cur.data
                        ->b[swicc_state->internal.apdu_cur.data->len],
                   swicc_state->buf_rx, swicc_state->buf_rx_len);
            /**
             * Safe cast due to 'if' condition that checks for data max
             * overflow.
             */
            swicc_state->internal.apdu_cur.data->len =
                (uint16_t)(swicc_state->internal.apdu_cur.data->len +
                           swicc_state->buf_rx_len);

            /* After receiving data, give back a procedure. */
            swicc_state->internal.tpdu_processed = true;
            swicc_state->internal.fsm_state = SWICC_FSM_STATE_CMD_PROCEDURE;
            swicc_state->buf_tx_len = 0U;

            /**
             * No data is expected between receiving the command data and
             * sending a procedure.
             */
            swicc_state->buf_rx_len = 0U;
            return;
        }

        /* Contact state is still fine so just return to waiting for command. */
        swicc_state->internal.tpdu_processed = true;
        swicc_state->buf_tx_len = 0U;
        swicc_state->buf_rx_len = 5U; /* Receive a new header. */
        swicc_state->internal.fsm_state = SWICC_FSM_STATE_CMD_WAIT;
        return;
    }
    swicc_state->internal.fsm_state = SWICC_FSM_STATE_OFF;
    swicc_state->buf_tx_len = 0U;
    swicc_state->buf_rx_len = 0U;
    return;
}

static swicc_fsmh_ft *const swicc_fsmh[] = {
    [SWICC_FSM_STATE_OFF] = fsm_handle_s_off,
    [SWICC_FSM_STATE_ACTIVATION] = fsm_handle_s_activation,
    [SWICC_FSM_STATE_RESET_COLD] = fsm_handle_s_reset_cold,
    [SWICC_FSM_STATE_ATR_REQ] = fsm_handle_s_atr_req,
    [SWICC_FSM_STATE_ATR_RES] = fsm_handle_s_atr_res,
    [SWICC_FSM_STATE_RESET_WARM] = fsm_handle_s_reset_warm,
    [SWICC_FSM_STATE_PPS_REQ] = fsm_handle_s_pps_req,
    [SWICC_FSM_STATE_CMD_WAIT] = fsm_handle_s_cmd_wait,
    [SWICC_FSM_STATE_CMD_PROCEDURE] = fsm_handle_s_cmd_procedure,
    [SWICC_FSM_STATE_CMD_DATA] = fsm_handle_s_cmd_data,
};

void swicc_fsm(swicc_st *const swicc_state)
{
    swicc_fsmh[swicc_state->internal.fsm_state](swicc_state);
}

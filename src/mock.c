#include <string.h>
#include <swicc/swicc.h>

swicc_ret_et swicc_mock_reset_cold(swicc_st *const swicc_state,
                                   bool const mock_pps)
{
    static uint8_t const pps_req[] = {0xFF, 0x10, 0x94, 0x7B};
    static_assert(sizeof(pps_req) / sizeof(pps_req[0U] >= 2),
                  "PPS (req/res) must be at least 2 bytes long.");
    uint8_t const pps_req_len = sizeof(pps_req) / sizeof(pps_req[0U]);

    /* All contact states are set to valid. */
    swicc_state->cont_state_rx = SWICC_IO_CONT_VALID_ALL;

    uint16_t const buf_tx_size = SWICC_DATA_MAX;

    swicc_fsm_state_et state_fsm;

    /* First reset the ICC to begin with a known state. */
    if (swicc_reset(swicc_state) != SWICC_RET_SUCCESS)
    {
        return SWICC_RET_ERROR;
    }

    for (uint8_t step = 0U;; ++step)
    {
        /* Setup buffers and user-controlled state before calling swICC IO. */
        switch (step)
        {
        case 0:
            /* 0. Interface sets RST to state L. */
            swicc_state->cont_state_rx &= ~((uint32_t)SWICC_IO_CONT_RST);
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 1:
            /* 1. Interface sets VCC to state COC. */
            swicc_state->cont_state_rx |= SWICC_IO_CONT_VCC;
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 2:
            /* 2. Interface sets IO to state H (reception mode). */
            swicc_state->cont_state_rx |= SWICC_IO_CONT_IO;
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 3:
            /* 3. Interface enables CLK. */
            swicc_state->cont_state_rx |= SWICC_IO_CONT_CLK;
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 4:
            /**
             * 4. Interface sets RST to state H to indicate to card that it
             * wants the ATR.
             */
            swicc_state->cont_state_rx |= SWICC_IO_CONT_RST;
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 5:
            /* 5. Card sends ATR to the interface. */
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 6:
            /**
             * 6. Card receives a PPS message header (0xFF) which starts a PPS
             * exchange.
             */
            swicc_state->buf_rx[0U] = pps_req[0U];
            swicc_state->buf_rx_len = 1U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 7:
            /**
             * 7. Card transitions to the PPS request state and will request all
             * bytes until (and including) PP0.
             */
            swicc_state->buf_rx_len = 0U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 8:
            /**
             * 8. Card receives the second byte (PPS0) of the PPS message and
             * now realizes how long the whole PPS message is.
             */
            swicc_state->buf_rx[0U] = pps_req[1U];
            swicc_state->buf_rx_len = 1U;
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        case 9:
            /**
             * 9. Card receives the remaining bytes of the PPS message and sends
             * back a PPS response.
             */
            memcpy(swicc_state->buf_rx, &pps_req[2U], pps_req_len - 2U);
            /* Safe cast since this is just 2. */
            swicc_state->buf_rx_len = (uint8_t)(pps_req_len - 2U);
            swicc_state->buf_tx_len = buf_tx_size;
            break;
        }

        /* Push the data to swICC IO. */
        swicc_io(swicc_state);
        swicc_fsm_state(swicc_state, &state_fsm);

        /* Decide if transition was done exactly like expected. */
        bool state_change_invalid = true;
        switch (step)
        {
        case 0:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_OFF ||
                                   swicc_state->buf_rx_len != 0U;
            break;
        case 1:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_ACTIVATION ||
                                   swicc_state->buf_rx_len != 0U;
            break;
        case 2:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_ACTIVATION ||
                                   swicc_state->buf_rx_len != 0U;
            break;
        case 3:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_RESET_COLD ||
                                   swicc_state->buf_rx_len != 0U;

            break;
        case 4:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_ATR_REQ ||
                                   swicc_state->buf_rx_len != 0U;
            break;
        case 5:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_ATR_RES ||
                                   swicc_state->buf_rx_len != 1U;
            break;
        case 6:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_PPS_REQ ||
                                   swicc_state->buf_rx_len != 0U;
            break;
        case 7:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_PPS_REQ ||
                                   swicc_state->buf_rx_len != 1U;
            break;
        case 8: {
            state_change_invalid = state_fsm != SWICC_FSM_STATE_PPS_REQ ||
                                   swicc_state->buf_rx_len != pps_req_len - 2U;
            break;
        }
        case 9:
            state_change_invalid = state_fsm != SWICC_FSM_STATE_CMD_WAIT ||
                                   swicc_state->buf_rx_len != 0U;
            break;
        }

        /* If transition failed, quit early. */
        if (state_change_invalid)
        {
            /* Step failed. */
            return SWICC_RET_ERROR;
        }

        /* Perform any extra tasks before the end of this step. */
        switch (step)
        {
        case 5:
            /**
             * @todo Can print ATR string here.
             */
            if (!mock_pps)
            {
                return SWICC_RET_SUCCESS;
            }
            break;
        case 9:
            /**
             * @todo Can print PPS string here.
             */
            return SWICC_RET_SUCCESS; /* Step 9 is the last step. */
        }
    }
}

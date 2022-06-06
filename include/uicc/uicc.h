#pragma once

#include "uicc/apdu.h"
#include "uicc/apduh.h"
#include "uicc/atr.h"
#include "uicc/dato.h"
#include "uicc/dbg.h"
#include "uicc/fs.h"
#include "uicc/fsm.h"
#include "uicc/io.h"
#include "uicc/pps.h"
#include "uicc/tpdu.h"

/* For holding transmission protocol configuration. */
typedef struct uicc_tp_s
{
    /**
     * ETU is the elementary time unit (ISO 7816-3:2006 p.13 sec.7.1)
     * and it dictates how many clock cycles will be used to transmit
     * each 'moment' of a character frame which consists of 10 moments.
     */
    uint32_t etu;

    /**
     * Fi, f(max), and Di are parameters of the transmission protocol.
     */
    uint16_t fi;
    uint32_t fmax;
    uint8_t di;
} uicc_tp_st;

/* Anything that is part of the file system is held here. */
typedef struct uicc_fs_s
{
    uicc_va_st va;
    uicc_disk_st disk;
} uicc_fs_st;

typedef struct uicc_s
{
    /**
     * State of the contacts as seen by the SIM.
     */
    uint32_t cont_state_rx;
    /**
     * Expected state of contacts as requested by UICC.
     */
    uint32_t cont_state_tx;
    /**
     * Receive data into this buffer.
     */
    uint8_t *buf_rx;
    /**
     * Before call to IO, shall hold the length of the RX buffer. After IO it
     * will receive the next length of data that should be read next.
     */
    uint16_t buf_rx_len;
    /**
     * UICC may request transmission of data to the interface. This buffer
     * receives that data.
     */
    uint8_t *buf_tx;
    /**
     * Length of the TX buffer. It must contain the maximum size of the TX
     * buffer before calling IO and it will receive the len requested to be
     * transmitted.
     */
    uint16_t buf_tx_len;

    /**
     * This needs to be outside of internal since it may be needed for
     * instruction implementation in the proprietary class.
     */
    uicc_fs_st fs;

    /* This shall not be modified by anything other than the UICC library. */
    struct
    {
        /**
         * Store the actively handled APDU command. Seems like there is no way
         * to handle APDUs without copying from the RX buffer...
         */
        uicc_tpdu_cmd_st tpdu_cur;
        uicc_apdu_cmd_st apdu_cur;

        /**
         * Receiving the header in parts is possible and while incomplete, is
         * held in this temporary buffer. This is cleared only after the command
         * is completely processed and another one is expected to arrive.
         */
        uint8_t tpdu_hdr[sizeof(uicc_apdu_cmd_hdr_raw_st) +
                         1U /* P3 (only part of TPDU header) */];
        uint8_t tpdu_hdr_len;

        /* True when the 'current' TPDU has already been processed. */
        bool tpdu_processed;

        /* Keep track of the received PPS. */
        uint8_t pps[UICC_PPS_LEN_MAX];
        uint8_t pps_len;

        /**
         * How many procedure bytes have been sent since receiving the header
         * (i.e. since the SIM started handling this command).
         */
        uint32_t procedure_count;

        uicc_fsm_state_et fsm_state;

        uicc_tp_st tp;
        uicc_apduh_ft *apduh_pro; /* For all proprietary classes. */

        /**
         * Used for the GET RESPONSE instruction (part of transmission
         * handling). All instructions write their data output to this buffer
         * (if it does not fit in the response being sent back from that
         * command or if response chaining is being used unconditionally).
         */
        struct
        {
            uint8_t b[UICC_DATA_MAX];
            uint16_t len;

            /* How much of the data was already returned to the interface. */
            uint16_t offset;
        } res;
    } internal;
} uicc_st;

#pragma once

#include "swicc/apdu.h"
#include "swicc/apduh.h"
#include "swicc/atr.h"
#include "swicc/dato.h"
#include "swicc/dbg.h"
#include "swicc/fs.h"
#include "swicc/fsm.h"
#include "swicc/io.h"
#include "swicc/pps.h"
#include "swicc/tpdu.h"

/* For holding transmission protocol configuration. */
typedef struct swicc_tp_s
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
} swicc_tp_st;

/* Anything that is part of the file system is held here. */
typedef struct swicc_fs_s
{
    swicc_va_st va;
    swicc_disk_st disk;
} swicc_fs_st;

typedef struct swicc_s
{
    /**
     * State of the contacts as seen by the SIM.
     */
    uint32_t cont_state_rx;
    /**
     * Expected state of contacts as requested by swICC.
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
     * swICC may request transmission of data to the interface. This buffer
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
     * These need to be outside of internal because may be needed for
     * instruction implementation in the proprietary class.
     */
    swicc_fs_st fs;
    swicc_apdu_rc_st apdu_rc;

    /* This shall not be modified by anything other than the swICC library. */
    struct
    {
        /**
         * Store the actively handled APDU command. Seems like there is no way
         * to handle APDUs without copying from the RX buffer...
         */
        swicc_tpdu_cmd_st tpdu_cur;
        swicc_apdu_cmd_st apdu_cur;

        /**
         * Receiving the header in parts is possible and while incomplete, is
         * held in this temporary buffer. This is cleared only after the command
         * is completely processed and another one is expected to arrive.
         */
        uint8_t tpdu_hdr[sizeof(swicc_apdu_cmd_hdr_raw_st) +
                         1U /* P3 (only part of TPDU header) */];
        uint8_t tpdu_hdr_len;

        /* True when the 'current' TPDU has already been processed. */
        bool tpdu_processed;

        /* Keep track of the received PPS. */
        uint8_t pps[SWICC_PPS_LEN_MAX];
        uint8_t pps_len;

        /**
         * How many procedure bytes have been sent since receiving the header
         * (i.e. since the SIM started handling this command).
         */
        uint32_t procedure_count;

        swicc_fsm_state_et fsm_state;

        swicc_tp_st tp;

        swicc_apduh_ft *apduh_pro; /* For all proprietary classes. */
    } internal;
} swicc_st;

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

typedef struct uicc_fs_s
{
    uicc_va_st va;
    uicc_disk_st disk;
} uicc_fs_st;

typedef struct uicc_s
{
    uint32_t cont_state_rx; /* State of contacts as they are now */
    uint32_t cont_state_tx; /* State of the contacts as they need to be */
    uint8_t *buf_rx;
    uint16_t buf_rx_len;
    uint8_t *buf_tx;
    uint16_t buf_tx_len;
    struct uicc_internal_s
    {
        /* Store the header of the actively handled APDU command. */
        struct
        {
            uicc_apdu_cmd_hdr_st hdr;
            uint8_t p3;
        } apdu_cur;

        uicc_fsm_state_et fsm_state;

        uicc_tp_st tp;
        uicc_fs_st fs;
        uicc_apduh_ft *apduh_pro; /* For all proprietary classes. */

        /**
         * Used for the GET RESPONSE instruction (part of transmission
         * handling). All instructions write their data output to this buffer
         * (if it does not fit in the response being sent back from that
         * command).
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

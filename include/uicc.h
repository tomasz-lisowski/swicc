#pragma once

#include "uicc/apdu.h"
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
    uicc_fs_disk_st disk;
    uicc_fs_va_st va;
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
        uicc_apdu_cmd_hdr_st
            apdu_cmd_hdr_cur; /* Store the header of the actively handled APDU
                                 command. */
        uicc_fsm_state_et fsm_state;
        uicc_tp_st tp;
        uicc_fs_st fs;
        uicc_apdu_h_ft *handle_pro; /* For all proprietary classes. */
    } internal;
} uicc_st;

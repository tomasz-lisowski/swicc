#include <usim/apdu.h>
#include <usim/atr.h>
#include <usim/dbg.h>
#include <usim/fsm.h>
#include <usim/io.h>
#include <usim/pps.h>
#include <usim/tpdu.h>

#pragma once

typedef struct usim_s
{
    struct internal_s
    {
        /**
         * ETU is the elementary time unit (ISO 7816-3:2006 p.13 sec.7.1) and it
         * dictates how many clock cycles will be used to transmit each 'moment'
         * of a character frame which consists of 10 moments.
         */
        uint32_t etu;
        /**
         * Fi, f(max), and Di are parameters of the transmission protocol.
         */
        uint16_t fi;
        uint32_t fmax;
        uint8_t di;

        uint8_t fsm_state; /* member of usim_fsm_state_et */
    } internal;
    uint32_t cont_state_rx; /* State of contacts as they are now */
    uint32_t cont_state_tx; /* State of the contacts as they need to be */
    uint8_t *buf_rx;
    uint16_t buf_rx_len;
    uint8_t *buf_tx;
    uint16_t buf_tx_len;
} usim_st;

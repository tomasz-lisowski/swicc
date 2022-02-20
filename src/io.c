#include "usim.h"
#include <string.h>

/**
 * A lookup table for the Fi (clock rate conversion integer) parameter.
 * Described in a table by ISO 7816-3:2006 p.18 sec.8.3.
 */
uint16_t const usim_lookup_fi[USIM_TP_CONF_NUM] = {
    [0b0000] = 372,         [0b0001] = 372,         [0b0010] = 558,
    [0b0011] = 744,         [0b0100] = 1116,        [0b0101] = 1488,
    [0b0110] = 1860,        [0b0111] = 0 /* RFU */, [0b1000] = 0 /* RFU */,
    [0b1001] = 512,         [0b1010] = 768,         [0b1011] = 1024,
    [0b1100] = 1536,        [0b1101] = 2048,        [0b1110] = 0 /* RFU */,
    [0b1111] = 0 /* RFU */,
};

/**
 * A lookup table for the Di (baud rate adjustment integer) parameter.
 * Described in a table by ISO 7816-3:2006 p.18 sec.8.3.
 */
uint8_t const usim_lookup_di[USIM_TP_CONF_NUM] = {
    [0b0000] = 0 /* RFU */, [0b0001] = 1,           [0b0010] = 2,
    [0b0011] = 4,           [0b0100] = 8,           [0b0101] = 16,
    [0b0110] = 32,          [0b0111] = 64,          [0b1000] = 12,
    [0b1001] = 20,          [0b1010] = 0 /* RFU */, [0b1011] = 0 /* RFU */,
    [0b1100] = 0 /* RFU */, [0b1101] = 0 /* RFU */, [0b1110] = 0 /* RFU */,
    [0b1111] = 0 /* RFU */,
};

/**
 * A lookup table for the f(max) (maximum supported clock frequency) parameter.
 * Described in a table by ISO 7816-3:2006 p.19 sec.8.3.
 */
uint32_t const usim_lookup_fmax[USIM_TP_CONF_NUM] = {
    [0b0000] = 4000,        [0b0001] = 5000,        [0b0010] = 6000,
    [0b0011] = 8000,        [0b0100] = 12000,       [0b0101] = 16000,
    [0b0110] = 20000,       [0b0111] = 0 /* RFU */, [0b1000] = 0 /* RFU */,
    [0b1001] = 5000,        [0b1010] = 7500,        [0b1011] = 10000,
    [0b1100] = 15000,       [0b1101] = 20000,       [0b1110] = 0 /* RFU */,
    [0b1111] = 0 /* RFU */,
};

usim_ret_et usim_io(usim_st *const usim_state)
{
    usim_ret_et ret = usim_fsm(usim_state);
    return ret;
}

usim_ret_et usim_reset(usim_st *const usim_state)
{
    memset(usim_state, 0U, sizeof(usim_st));
    usim_state->internal.fsm_state = USIM_FSM_STATE_OFF;
    usim_state->internal.fi = usim_lookup_fi[USIM_TP_CONF_DEFAULT];
    usim_state->internal.di = usim_lookup_di[USIM_TP_CONF_DEFAULT];
    usim_state->internal.fmax = usim_lookup_fmax[USIM_TP_CONF_DEFAULT];
    usim_etu(&usim_state->internal.etu, usim_lookup_fi[USIM_TP_CONF_DEFAULT],
             usim_lookup_di[USIM_TP_CONF_DEFAULT],
             usim_lookup_fmax[USIM_TP_CONF_DEFAULT]);
    return USIM_RET_UNKNOWN;
}

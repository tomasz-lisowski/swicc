#include "uicc.h"
#include <assert.h>

void uicc_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
              uint32_t const fmax)
{
    assert(fmax != 0U);
    assert(di != 0U);
    *etu = fi / (di * fmax);
}

uint8_t uicc_tck(uint8_t const *const buf_raw, uint16_t const buf_raw_len)
{
    uint8_t tck = false;
    for (uint16_t buf_idx = 0U; buf_idx < buf_raw_len; ++buf_idx)
    {
        tck ^= buf_raw[buf_idx];
    }
    return tck;
}

uicc_ret_et uicc_reset(uicc_st *const uicc_state)
{
    uicc_ret_et ret = UICC_RET_UNKNOWN;
    ret = uicc_fs_reset(uicc_state);
    if (ret != UICC_RET_SUCCESS)
    {
        return ret;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->internal.tp.fi = uicc_io_fi_arr[UICC_TP_CONF_DEFAULT];
    uicc_state->internal.tp.di = uicc_io_di_arr[UICC_TP_CONF_DEFAULT];
    uicc_state->internal.tp.fmax = uicc_io_fmax_arr[UICC_TP_CONF_DEFAULT];
    uicc_etu(&uicc_state->internal.tp.etu, uicc_io_fi_arr[UICC_TP_CONF_DEFAULT],
             uicc_io_di_arr[UICC_TP_CONF_DEFAULT],
             uicc_io_fmax_arr[UICC_TP_CONF_DEFAULT]);
    return UICC_RET_SUCCESS;
}

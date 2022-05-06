#include "uicc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

uicc_ret_et uicc_hexstr_bytearr(char const *const hexstr,
                                uint32_t const hexstr_len,
                                uint8_t *const bytearr,
                                uint32_t *const bytearr_len)
{
    if (hexstr_len % 2U != 0U)
    {
        /* Hex string must be even. */
        return UICC_RET_PARAM_BAD;
    }
    for (uint32_t hexstr_idx = 0U; hexstr_idx < hexstr_len; hexstr_idx += 2U)
    {
        if ((hexstr_idx / 2U) >= *bytearr_len)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        uint8_t nibble[2U] = {(uint8_t)hexstr[hexstr_idx + 0U],
                              (uint8_t)hexstr[hexstr_idx + 1U]};
        for (uint8_t nibble_idx = 0U; nibble_idx < 2U; ++nibble_idx)
        {
            if (nibble[nibble_idx] >= '0' && nibble[nibble_idx] <= '9')
            {
                /* Safe cast due to range check. */
                nibble[nibble_idx] = (uint8_t)(nibble[nibble_idx] - '0');
            }
            else if (nibble[nibble_idx] >= 'A' && nibble[nibble_idx] <= 'F')
            {
                /* Safe cast due to range check. */
                nibble[nibble_idx] =
                    (uint8_t)(0x0A + (nibble[nibble_idx] - 'A'));
            }
            else
            {
                return UICC_RET_PARAM_BAD;
            }
        }
        /**
         * There are twice as many nibbles as bytes so we divide hex string
         * index by 2 to get byte array index.
         */
        bytearr[hexstr_idx / 2U] =
            (uint8_t)((nibble[0U] << 4) | nibble[1U]); /* Safe case due to range
                                                          check on nibble. */
    }
    *bytearr_len = hexstr_len / 2U;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_reset(uicc_st *const uicc_state)
{
    uicc_ret_et ret = UICC_RET_ERROR;
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

uicc_ret_et uicc_terminate(uicc_st *const uicc_state)
{
    uicc_ret_et ret = uicc_reset(uicc_state);
    if (ret != UICC_RET_SUCCESS)
    {
        return ret;
    }
    uicc_fs_disk_unload(uicc_state);
    ret = UICC_RET_SUCCESS;
    return ret;
}

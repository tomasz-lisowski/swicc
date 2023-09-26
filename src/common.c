#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <swicc/swicc.h>

void swicc_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
               uint32_t const fmax)
{
    assert(fmax != 0U);
    assert(di != 0U);
    *etu = fi / (di * fmax);
}

uint8_t swicc_ck(uint8_t const *const buf_raw, uint16_t const buf_raw_len)
{
    uint8_t tck = 0U;
    for (uint16_t buf_idx = 0U; buf_idx < buf_raw_len; ++buf_idx)
    {
        tck ^= buf_raw[buf_idx];
    }
    return tck;
}

swicc_ret_et swicc_hexstr_bytearr(char const *const hexstr,
                                  uint32_t const hexstr_len,
                                  uint8_t *const bytearr,
                                  uint32_t *const bytearr_len)
{
    if (hexstr_len % 2U != 0U)
    {
        /* Hex string must be even. */
        return SWICC_RET_PARAM_BAD;
    }
    for (uint32_t hexstr_idx = 0U; hexstr_idx < hexstr_len; hexstr_idx += 2U)
    {
        if ((hexstr_idx / 2U) > *bytearr_len)
        {
            return SWICC_RET_BUFFER_TOO_SHORT;
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
                return SWICC_RET_PARAM_BAD;
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
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_reset(swicc_st *const swicc_state)
{
    swicc_ret_et ret = SWICC_RET_ERROR;
    ret = swicc_va_reset(&swicc_state->fs);
    if (ret != SWICC_RET_SUCCESS)
    {
        return ret;
    }

    swicc_apduh_ft *const apduh_pro = swicc_state->internal.apduh_pro;
    swicc_apduh_ft *const apduh_override = swicc_state->internal.apduh_override;
    memset(&swicc_state->internal, 0U, sizeof(swicc_state->internal));
    memset(&swicc_state->apdu_rc, 0U, sizeof(swicc_state->apdu_rc));
    memset(swicc_state->buf_tx, 0U, sizeof(*swicc_state->buf_tx));
    swicc_state->buf_tx_len = 0U;
    swicc_state->cont_state_tx = 0U;
    swicc_state->internal.apduh_pro = apduh_pro;
    swicc_state->internal.apduh_override = apduh_override;

    return SWICC_RET_SUCCESS;
}

void swicc_terminate(swicc_st *const swicc_state)
{
    swicc_disk_unload(&swicc_state->fs.disk);
}

void swicc_fsm_state(swicc_st *const swicc_state,
                     swicc_fsm_state_et *const state)
{
    *state = swicc_state->internal.fsm_state;
}

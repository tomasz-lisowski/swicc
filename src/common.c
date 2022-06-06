#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uicc/uicc.h>

void uicc_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
              uint32_t const fmax)
{
    assert(fmax != 0U);
    assert(di != 0U);
    *etu = fi / (di * fmax);
}

uint8_t uicc_ck(uint8_t const *const buf_raw, uint16_t const buf_raw_len)
{
    uint8_t tck = 0U;
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
    ret = uicc_va_reset(&uicc_state->fs);
    if (ret != UICC_RET_SUCCESS)
    {
        return ret;
    }
    uicc_state->internal.fsm_state = UICC_FSM_STATE_OFF;
    uicc_state->internal.tp.fi = uicc_io_fi[UICC_TP_CONF_DEFAULT];
    uicc_state->internal.tp.di = uicc_io_di[UICC_TP_CONF_DEFAULT];
    uicc_state->internal.tp.fmax = uicc_io_fmax[UICC_TP_CONF_DEFAULT];
    uicc_etu(&uicc_state->internal.tp.etu, uicc_io_fi[UICC_TP_CONF_DEFAULT],
             uicc_io_di[UICC_TP_CONF_DEFAULT],
             uicc_io_fmax[UICC_TP_CONF_DEFAULT]);
    memset(&uicc_state->internal.res, 0U, sizeof(uicc_state->internal.res));
    return UICC_RET_SUCCESS;
}

void uicc_terminate(uicc_st *const uicc_state)
{
    uicc_disk_unload(&uicc_state->fs.disk);
}

uicc_ret_et uicc_file_lcs(uicc_fs_file_st const *const file, uint8_t *const lcs)
{
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }
    switch (file->hdr_item.lcs)
    {
    // case UICC_FS_LCS_NINFO:
    //     *lcs = 0b00000000;
    //     break;
    // case UICC_FS_LCS_CREAT:
    //     *lcs = 0b00000001;
    //     break;
    // case UICC_FS_LCS_INIT:
    //     *lcs = 0b00000011;
    //     break;
    case UICC_FS_LCS_OPER_ACTIV:
        *lcs = 0b00000101;
        break;
    case UICC_FS_LCS_OPER_DEACTIV:
        *lcs = 0b00000100;
        break;
    case UICC_FS_LCS_TERM:
        *lcs = 0b00001100;
        break;
    }
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_file_descr(uicc_fs_file_st const *const file,
                            uint8_t *const file_descr)
{
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }
    /**
     * 0
     *  0       = File not shareable
     *   xxx    = DF or EF category
     *      xxx = 0 for DF or EF structure
     */
    *file_descr = 0b00000000;
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_MF ||
        file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_ADF ||
        file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_DF)
    {
        *file_descr |= 0b00111000;
    }
    else
    {
        /**
         * TODO: Not sure what working and internal EFs are so just setting it
         * to "internal EF" i.e. EF for storing data interpreted by the card.
         */
        *file_descr |= 0b00001000;
        switch (file->hdr_item.type)
        {
        case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
            *file_descr |= 0b00000001;
            break;
        case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
            *file_descr |= 0b00000010;
            break;
        case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
            *file_descr |= 0b00000110;
            break;
        default:
            return UICC_RET_PARAM_BAD;
        }
    }
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_file_data_coding(uicc_fs_file_st const *const file,
                                  uint8_t *const data_coding)
{
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }
    /**
     * 0        = EFs of BER-TLV structure supported.
     *  01      = Behavior of write function is proprietary.
     *    0     = Value 'FF' for first byte of BER-TLV tag fields is invalid.
     *     0001 = Data unit size is 2 quartet = 1 byte.
     */
    *data_coding = 0b00100001;
    return UICC_RET_SUCCESS;
}

void uicc_fsm_state(uicc_st *const uicc_state, uicc_fsm_state_et *const state)
{
    *state = uicc_state->internal.fsm_state;
}

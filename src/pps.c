#include <uicc/uicc.h>
#include <string.h>

static uint8_t pps_pck(uint8_t const *const buf_rx, uint16_t const buf_rx_len)
{
    assert(buf_rx_len <= UINT8_MAX);
    uint8_t pck = 0U;
    for (uint8_t rx_idx = 0U; rx_idx < buf_rx_len; ++rx_idx)
    {
        pck ^= buf_rx[rx_idx];
    }
    return pck;
}

static uicc_ret_et pps_xchg_success(uint8_t const *const buf_rx,
                                    uint16_t const buf_rx_len,
                                    uint8_t const *const buf_tx,
                                    uint16_t const buf_tx_len)
{
    if (buf_tx_len == buf_rx_len && memcmp(buf_tx, buf_rx, buf_tx_len) == 0)
    {
        return UICC_RET_SUCCESS;
    }
    else
    {
        /**
         * The interface sent parameters that are not supported by the card so
         * the PPS exchange is not done.
         */
        return UICC_RET_PPS_FAILED;
    }
}

static uicc_ret_et pps_parse(uicc_pps_params_st *const pps_params,
                             uint8_t const *const buf_rx,
                             uint16_t const buf_rx_len)
{
    if (buf_rx_len < 2U || buf_rx_len > 6U || buf_rx[0U] != UICC_PPS_PPSS ||
        pps_pck(buf_rx, buf_rx_len) != 0U)
    {
        return UICC_RET_PPS_INVALID;
    }

    uint8_t const pps0 = buf_rx[1U];
    if ((pps0 & 0b10000000) != 0U)
    {
        /* PPS0 bit 8 is RFU and should be 0 */
        return UICC_RET_PPS_INVALID;
    }
    uint8_t const t_proposed = pps0 & 0x0F;

    uint8_t buf_rx_idx_next = 2U;
    uint8_t pps_mask = 0b00010000;

    for (uint8_t pps_idx = 1U; pps_idx <= 3U; ++pps_idx)
    {
        /* PPSi is present */
        if ((pps0 & pps_mask) && buf_rx_idx_next < buf_rx_len)
        {
            uint8_t const ppsi = buf_rx[buf_rx_idx_next++];
            switch (pps_idx)
            {
            case 1:
                /* PPS1 */
                pps_params->fi_idx = ppsi & 0x0F;
                pps_params->di_idx = (ppsi & 0xF0) >> 4U;
                break;
            case 2:
                /* PPS2 */
                /* TODO: How is this encoded? */
                pps_params->spu = ppsi;
                break;
            case 3:
                /* PPS3 */
                if (ppsi != 0U)
                {
                    /* PPS3 is RFU and should be 0 */
                    return UICC_RET_PPS_INVALID;
                }
                break;
            }
        }
        else
        {
            if ((pps0 & pps_mask))
            {
                /**
                 * PPS0 indicated presence of the PPSi byte but RX buf is too
                 * short to contain it.
                 */
                return UICC_RET_PPS_INVALID;
            }
            else
            {
                /**
                 * Normal behavior that PPSi is missing if not indicated to be
                 * present in PPS0.
                 */
            }
        }
        pps_mask =
            (uint8_t)(pps_mask << 1U); /* Safe cast due to range of for loop. */
    }
    pps_params->t = t_proposed;
    return UICC_RET_SUCCESS;
}

static uicc_ret_et pps_deparse(uicc_pps_params_st *const pps_params,
                               uint8_t const pps0, uint8_t *const buf_tx,
                               uint16_t *const buf_tx_len)
{
    /* The PPS response message */
    uint8_t ppsi[UICC_PPS_LEN_MAX];
    uint8_t ppsi_next = 0U;
    ppsi[ppsi_next++] = UICC_PPS_PPSS;
    ppsi[ppsi_next++] = pps0; /* PPS0 */

    if ((pps_params->fi_idx == UICC_TP_CONF_DEFAULT &&
         pps_params->di_idx == UICC_TP_CONF_DEFAULT) ||
        (pps0 & 0b00010000) == 0 /* PPS1 was not present? */)
    {
        /**
         * The PPS request contained a PPS1 but the proposed value is being
         * ignored and the defaults shall continue to be used.
         */
        ppsi[1U] &= 0b11101111; /* PPS0 */
    }
    else
    {
        assert((pps_params->fi_idx & 0xF0) == 0U);
        assert((pps_params->di_idx & 0xF0) == 0U);
        ppsi[ppsi_next++] =
            (uint8_t)(pps_params->di_idx << 4U) | pps_params->fi_idx; /* PPS1 */
    }

    if (pps0 & 0b00100000)
    {
        /* PPS2 should be present */
        ppsi[ppsi_next++] = pps_params->spu; /* PPS2 */
    }
    if (pps0 & 0b01000000)
    {
        /* PPS3 should be present */
        ppsi[ppsi_next++] = 0; /* PPS3 is RFU and should be 0 */
    }

    if (ppsi_next > *buf_tx_len)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        /* Compute check byte for the PPS response */
        ppsi[ppsi_next] = pps_pck(ppsi, ppsi_next);
        ppsi_next++;

        memcpy(buf_tx, ppsi, ppsi_next);
        *buf_tx_len = ppsi_next;
        return UICC_RET_SUCCESS;
    }
}

uicc_ret_et uicc_pps(uicc_pps_params_st *const pps_params,
                     uint8_t const *const buf_rx, uint16_t const buf_rx_len,
                     uint8_t *const buf_tx, uint16_t *const buf_tx_len)
{
    uicc_ret_et ret = pps_parse(pps_params, buf_rx, buf_rx_len);
    if (ret != UICC_RET_SUCCESS)
    {
        return ret;
    }

    ret = pps_deparse(pps_params, buf_rx[1U], buf_tx, buf_tx_len);
    if (ret != UICC_RET_SUCCESS)
    {
        return ret;
    }

    ret = pps_xchg_success(buf_rx, buf_rx_len, buf_tx, *buf_tx_len);
    if (ret != UICC_RET_SUCCESS)
    {
        return ret;
    }
    return UICC_RET_SUCCESS;
}

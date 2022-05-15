#include "uicc.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct uicc_atr_chunk
{
    bool y[4U];
    uint8_t tabcd[4U];
    /**
     * Number of historical bytes.
     * ISO 7816-4:2020 p.16 sec.8.2.1 table.6.
     **/
    uint8_t k;
    /**
     * Tranmission protocol.
     * ISO 7816-4:2020 p.17 sec.8.2.3.
     **/
    uint8_t t;
} uicc_atr_chunk_t;

#ifdef DEBUG
static char const *const uicc_dbg_table_str_indicator[2U] = {"no", "yes"};
#endif /* DEBUG */

uicc_ret_et uicc_dbg_atr_str(char *const buf_str, uint16_t *const buf_str_len,
                             uint8_t const *const buf_atr,
                             uint16_t const buf_atr_len)
{
#ifdef DEBUG
    int bytes_written = 0U;
    int ret;

    if (*buf_str_len - bytes_written < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    ret = snprintf(buf_str + bytes_written,
                   *buf_str_len - (uint32_t)bytes_written, "(ATR ");
    if (ret < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    bytes_written += ret;

    if (buf_atr_len < 2U) /* TODO: Verify TS. */
    {
        if (*buf_str_len - bytes_written < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        ret = snprintf(buf_str + bytes_written,
                       *buf_str_len - (uint32_t)bytes_written, "??\?)\n");
        if (ret < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        return UICC_RET_ATR_INVALID;
    }

    bool chunk_last = false;
    uint8_t chunk_idx = 0U;    /* Indicates i for TAi, TBi, TCi, TDi. */
    uint16_t buf_atr_idx = 1U; /* Skip over TS. */
    uicc_atr_chunk_t chunk = {0}, chunk_prev = {0};

    /* To determine TCK presence. ISO 7816-3:2006 p.18 sec.8.2.5. */
    bool t0_present = false;
    bool t15_present = false;
    bool not_t0_or_t15_present = false;

    do
    {
        uint8_t chunk_size = 0;

        /* Check if reached end of interface byte chunks. */
        if (chunk_idx > 0U && (chunk_prev.y[0U] + chunk_prev.y[1U] +
                               chunk_prev.y[2U] + chunk_prev.y[3U]) == 0)
        {
            /* No bytes in this chunk so end here. */
            chunk_last = true;
            break;
        }

        /* Prepare header of chunk. */
        if (*buf_str_len - bytes_written < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        if (chunk_idx == 0U)
        {
            ret = snprintf(buf_str + bytes_written,
                           *buf_str_len - (uint32_t)bytes_written,
                           "\n  (T%02u ", chunk_idx);
        }
        else
        {
            /* TI = Interface byte. */
            ret = snprintf(buf_str + bytes_written,
                           *buf_str_len - (uint32_t)bytes_written, "\n  (TI%u ",
                           chunk_idx - 1U);
        }
        if (ret < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        bytes_written += ret;

        /**
         * Structural as in the Y indicator because it indicates what interface
         * bytes (A, B, C, D or T0) are present in the current chunk.
         */
        bool const structural_present =
            chunk_prev.y[3U] == true || chunk_idx == 0U;
        /* Determine which interface bytes are absent/present in next chunk. */
        if (structural_present)
        {
            /* Safe cast since this is at most 3. */
            uint8_t const td_offset =
                (uint8_t)(chunk_prev.y[0U] + chunk_prev.y[1U] +
                          chunk_prev.y[2U]);
            uint8_t const y = buf_atr[buf_atr_idx + td_offset] >> 4U;
            uint8_t const k_or_t = buf_atr[buf_atr_idx + td_offset] & 0x0F;
            uint8_t y_mask = 0b0001;

            for (uint8_t y_idx = 0U; y_idx < 4U; ++y_idx)
            {
                /* Read the Y indicator for next chunk. */
                chunk.y[y_idx] = (y & y_mask) > 0U;
                /* Safe cast because we loop <7 times. */
                y_mask = (uint8_t)(y_mask << 1U);
            }

            /* Save K/T from the structural byte. */
            if (chunk_idx == 0U)
            {
                chunk.k = k_or_t;
            }
            else
            {
                chunk.t = k_or_t;
                if (chunk.t == 0)
                {
                    t0_present |= true;
                }
                else if (chunk.t == 15)
                {
                    t15_present |= true;
                }
                else
                {
                    not_t0_or_t15_present |= true;
                }
            }

            /* Check if next chunk exists or not. */
            if (chunk_idx == 0U && y == 0)
            {
                /* Chunk 0 indicates there are no further interface bytes. */
                chunk_last = true;
            }
        }
        else
        {
            /* If no TD exists in current chunk, assume Y=0 i.e. no next chunk.
             */
            memset(chunk.y, 0, sizeof(chunk.y));
        }

        if (chunk_idx == 0U)
        {
            /* Skip over T0 once it's parsed. */
            /* Safe cast since chunk size is never larger than 4. */
            chunk_size = (uint8_t)(chunk_size + 1U);
        }
        else
        {
            /* Print all present interface bytes for chunks >=1. */
            for (uint8_t y_idx = 0U; y_idx < 4U; ++y_idx)
            {
                /* Chunk 0 contains exactly 1 byte, i.e. a format byte T0. */
                if (chunk_prev.y[y_idx] == true)
                {
                    chunk.tabcd[y_idx] = buf_atr[buf_atr_idx + chunk_size];
                    if (*buf_str_len - bytes_written < 0)
                    {
                        return UICC_RET_BUFFER_TOO_SHORT;
                    }
                    ret = snprintf(buf_str + bytes_written,
                                   *buf_str_len - (uint32_t)bytes_written,
                                   "\n    (T%c%u 0x%02X)", 'A' + y_idx,
                                   chunk_idx, chunk.tabcd[y_idx]);
                    if (ret < 0)
                    {
                        return UICC_RET_BUFFER_TOO_SHORT;
                    }
                    bytes_written += ret;
                    /* Safe cast since chunk size is never larger than 4. */
                    chunk_size = (uint8_t)(chunk_size + 1U);
                }
                else
                {
                    chunk.tabcd[y_idx] = 0U;
                }
            }
        }

        /**
         * The previous chunk determines what interface bytes are printed for
         * the current chunk so this needs to be saved.
         */
        memcpy(&chunk_prev, &chunk, sizeof(uicc_atr_chunk_t));

        /* Print the Y indicator, T (or K for chunk 0), and trailer. */
        if (*buf_str_len - bytes_written < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        if (chunk_last || !structural_present)
        {
            ret = 0;
        }
        else
        {
            ret = snprintf(
                buf_str + bytes_written, *buf_str_len - (uint32_t)bytes_written,
                "\n    (Y%u (A '%s') (B '%s') (C '%s') (D '%s'))",
                chunk_idx + 1U, uicc_dbg_table_str_indicator[chunk.y[0U]],
                uicc_dbg_table_str_indicator[chunk.y[1U]],
                uicc_dbg_table_str_indicator[chunk.y[2U]],
                uicc_dbg_table_str_indicator[chunk.y[3U]]);
        }
        if (ret < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        bytes_written += ret;

        /* Print K (historical byte count) or T (protocol type). */
        if (*buf_str_len - bytes_written < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        if (chunk_last || !structural_present)
        {
            ret = snprintf(buf_str + bytes_written,
                           *buf_str_len - (uint32_t)bytes_written, ")");
        }
        else
        {
            if (chunk_idx > 0)
            {
                ret = snprintf(buf_str + bytes_written,
                               *buf_str_len - (uint32_t)bytes_written,
                               "\n    (T%u %u))", chunk_idx + 1U, chunk.t);
            }
            else
            {
                ret = snprintf(buf_str + bytes_written,
                               *buf_str_len - (uint32_t)bytes_written,
                               "\n    (K %u))", chunk.k);
            }
        }
        if (ret < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        bytes_written += ret;

        /* Done parsing current chunk. */
        chunk_idx++;
        if (buf_atr_idx + chunk_size > UINT16_MAX)
        {
            return UICC_RET_ATR_INVALID;
        }
        /* Safe cast due to check above. */
        buf_atr_idx = (uint16_t)(buf_atr_idx + chunk_size);
    } while (buf_atr_idx < (buf_atr_len - 1 /* TCK */) && chunk_last == false);

    /* Print historical bytes (if any) here. */
    for (uint8_t hist_idx = 0U; hist_idx < chunk.k; ++hist_idx)
    {
        if (*buf_str_len - bytes_written < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        ret = snprintf(
            buf_str + bytes_written, *buf_str_len - (uint32_t)bytes_written,
            "\n  (T%02u 0x%02X)", hist_idx + 1U /* T0 is the format byte */,
            buf_atr[buf_atr_idx + hist_idx]);
        if (ret < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        bytes_written += ret;
    }
    /* Done printing the historical bytes. */
    if (buf_atr_idx + chunk.k > UINT16_MAX)
    {
        return UICC_RET_ATR_INVALID;
    }
    /* Safe cast due to check above. */
    buf_atr_idx = (uint16_t)(buf_atr_idx + chunk.k);

    /* Determine if TCK is present. */
    if (!(t0_present && !t15_present && !not_t0_or_t15_present))
    {
        if (*buf_str_len - bytes_written < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        if (buf_atr_idx >= buf_atr_len)
        {
            /* ATR does not contain a TCK when it should. */
            ret = snprintf(buf_str + bytes_written,
                           *buf_str_len - (uint32_t)bytes_written,
                           "\n  (TCK 'missing'))");
            return UICC_RET_ATR_INVALID;
        }
        else
        {
            /* Safe cast since at this point the length of ATR will be >= 1. */
            uint8_t const tck = uicc_tck(buf_atr + 1U /* Skip TS */,
                                         (uint8_t)(buf_atr_len - 1U));
            ret = snprintf(buf_str + bytes_written,
                           *buf_str_len - (uint32_t)bytes_written,
                           "\n  (TCK '%s'))", tck != 0 ? "invalid" : "valid");
        }
    }
    else
    {
        ret = snprintf(buf_str + bytes_written,
                       *buf_str_len - (uint32_t)bytes_written, ")");
    }
    if (ret < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    bytes_written += ret;

    *buf_str_len =
        (uint16_t)bytes_written; /* Safe cast due to args of snprintf. */
    return UICC_RET_SUCCESS;
#else
    return UICC_RET_SUCCESS;
#endif
}

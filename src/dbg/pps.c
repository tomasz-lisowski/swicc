#include "uicc.h"
#include <stdbool.h>
#include <stdio.h>

uicc_ret_et uicc_dbg_pps_str(char *const buf_str, uint16_t *const buf_str_len,
                             uint8_t const *const buf_pps,
                             uint16_t const buf_pps_len)
{
#ifdef DEBUG
    uint8_t const pps0 = buf_pps[1U];
    bool const pps1_present = pps0 & 0b00010000;
    bool const pps2_present = pps0 & 0b00100000;
    uint8_t buf_pps_next = 2U;

    uint16_t const di = pps1_present && buf_pps_next < buf_pps_len
                            ? uicc_io_di[buf_pps[buf_pps_next] & 0x0F]
                            : 0;
    uint16_t const fi = pps1_present && buf_pps_next < buf_pps_len
                            ? uicc_io_fi[(buf_pps[buf_pps_next] & 0xF0) >> 4U]
                            : 0;
    if (pps1_present)
    {
        ++buf_pps_next;
    }
    uint16_t const spu =
        pps2_present && buf_pps_next < buf_pps_len ? buf_pps[buf_pps_next] : 0;
    if (pps2_present)
    {
        ++buf_pps_next;
    }

    int bytes_written = snprintf(buf_str, *buf_str_len,
                                 // clang-format off
                                 "(PPS"
                                 "\n  (T %u)"
                                 "\n  (Fi %u) (Di %u)"
                                 "\n  (SPU %u))",
                                 // clang-format on
                                 pps0 & 0x0F, fi, di, spu);
    if (bytes_written < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        *buf_str_len =
            (uint16_t)bytes_written; /* Safe cast due to args of snprintf */
        return UICC_RET_SUCCESS;
    }
#else
    return UICC_RET_SUCCESS;
#endif
}

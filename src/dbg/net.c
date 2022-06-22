#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <swicc/swicc.h>

swicc_ret_et swicc_dbg_net_msg_str(char *const buf_str,
                                   uint16_t *const buf_str_len,
                                   char const *const prestr,
                                   swicc_net_msg_st const *const msg)
{
#ifdef DEBUG
    int32_t const len_base = snprintf(
        buf_str, *buf_str_len,
        "%s"
        // clang-format off
        "("CLR_KND("Message")
        "\n    ("CLR_KND("Header")" ("CLR_KND("Size")" "CLR_VAL("%u")"))"
        "\n    ("CLR_KND("Data")
        "\n        ("CLR_KND("Control")" "CLR_VAL("0x%02X")")"
        "\n        ("CLR_KND("Cont")" "CLR_VAL("0x%08X")")"
        "\n        ("CLR_KND("BufLenExp")" "CLR_VAL("%u")")"
        "\n        ("CLR_KND("Buf")" [",
        // clang-format on
        prestr, msg->hdr.size, msg->data.ctrl, msg->data.cont_state,
        msg->data.buf_len_exp);
    if (len_base < 0)
    {
        return SWICC_RET_ERROR;
    }
    else if (len_base > (int32_t)*buf_str_len || len_base >= UINT16_MAX)
    {
        return SWICC_RET_BUFFER_TOO_SHORT;
    }
    uint16_t len = (uint16_t)len_base;

    if (msg->hdr.size > sizeof(msg->data.buf) ||
        msg->hdr.size < offsetof(swicc_net_msg_data_st, buf))
    {
        char const str_inv[] = CLR_VAL("invalid");
        if (len + strlen(str_inv) > *buf_str_len)
        {
            return SWICC_RET_BUFFER_TOO_SHORT;
        }
        memcpy(&buf_str[len], str_inv, strlen(str_inv));
        /* Safe cast due to the check in the 'if'. */
        len = (uint16_t)(len + strlen(str_inv));
    }
    else
    {
        for (uint32_t data_idx = 0U;
             data_idx < msg->hdr.size - offsetof(swicc_net_msg_data_st, buf);
             ++data_idx)
        {
            /**
             * Safe cast since buffer string length is always greater or equal
             * to the length.
             */
            int32_t const len_extra =
                snprintf(&buf_str[len], (uint16_t)(*buf_str_len - len),
                         CLR_VAL(" %02X"), msg->data.buf[data_idx]);
            if (len_extra < 0)
            {
                return SWICC_RET_ERROR;
            }
            else if (len + len_extra > *buf_str_len ||
                     len + len_extra > UINT16_MAX)
            {
                return SWICC_RET_BUFFER_TOO_SHORT;
            }
            /* Safe cast since this was checked in the 'if'. */
            len = (uint16_t)(len + (uint16_t)len_extra);
        }
    }
    char const str_end[] = " ])))";
    if (len + strlen(str_end) > *buf_str_len)
    {
        return SWICC_RET_BUFFER_TOO_SHORT;
    }
    memcpy(&buf_str[len], str_end, sizeof(str_end));
    /* Safe cast since this was checked in the 'if'. */
    len = (uint16_t)(len + sizeof(str_end));
    *buf_str_len = len;
    return SWICC_RET_SUCCESS;
#else
    *buf_str_len = 0U;
    return SWICC_RET_SUCCESS;
#endif
}

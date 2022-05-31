#include <stdio.h>
#include <uicc/uicc.h>

static char const *const cont_str[] = {"H", "L", "F"};

uicc_ret_et uicc_dbg_io_cont_str(char *const buf_str,
                                 uint16_t *const buf_str_len,
                                 uint32_t const cont_state_rx,
                                 uint32_t const cont_state_tx)
{
#ifdef DEBUG
    int bytes_written = snprintf(
        buf_str, *buf_str_len,
        // clang-format off
        "("CLR_KND("Cont")
        "\n  ("CLR_KND("RX")" ("CLR_KND("C1")" "CLR_VAL("'%s'")") ("CLR_KND("C2")" "CLR_VAL("'%s'")") ("CLR_KND("C3")" "CLR_VAL("'%s'")") ("CLR_KND("C6")" "CLR_VAL("'%s'")") ("CLR_KND("C7")" "CLR_VAL("'%s'")"))"
        "\n  ("CLR_KND("TX")" ("CLR_KND("C1")" "CLR_VAL("'%s'")") ("CLR_KND("C2")" "CLR_VAL("'%s'")") ("CLR_KND("C3")" "CLR_VAL("'%s'")") ("CLR_KND("C6")" "CLR_VAL("'%s'")") ("CLR_KND("C7")" "CLR_VAL("'%s'")"))"
        ")\n",
        // clang-format on
        cont_state_rx & UICC_IO_CONT_VALID_C1
            ? cont_str[(cont_state_rx & UICC_IO_CONT_C1) > 0]
            : cont_str[2U],
        cont_state_rx & UICC_IO_CONT_VALID_C2
            ? cont_str[(cont_state_rx & UICC_IO_CONT_C2) > 0]
            : cont_str[2U],
        cont_state_rx & UICC_IO_CONT_VALID_C3
            ? cont_str[(cont_state_rx & UICC_IO_CONT_C3) > 0]
            : cont_str[2U],
        cont_state_rx & UICC_IO_CONT_VALID_C6
            ? cont_str[(cont_state_rx & UICC_IO_CONT_C6) > 0]
            : cont_str[2U],
        cont_state_rx & UICC_IO_CONT_VALID_C7
            ? cont_str[(cont_state_rx & UICC_IO_CONT_C7) > 0]
            : cont_str[2U],
        cont_state_tx & UICC_IO_CONT_VALID_C1
            ? cont_str[(cont_state_tx & UICC_IO_CONT_C1) > 0]
            : cont_str[2U],
        cont_state_tx & UICC_IO_CONT_VALID_C2
            ? cont_str[(cont_state_tx & UICC_IO_CONT_C2) > 0]
            : cont_str[2U],
        cont_state_tx & UICC_IO_CONT_VALID_C3
            ? cont_str[(cont_state_tx & UICC_IO_CONT_C3) > 0]
            : cont_str[2U],
        cont_state_tx & UICC_IO_CONT_VALID_C6
            ? cont_str[(cont_state_tx & UICC_IO_CONT_C6) > 0]
            : cont_str[2U],
        cont_state_tx & UICC_IO_CONT_VALID_C7
            ? cont_str[(cont_state_tx & UICC_IO_CONT_C7) > 0]
            : cont_str[2U]);
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
    *buf_str_len = 0U;
    return UICC_RET_SUCCESS;
#endif
}

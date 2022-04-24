#include "uicc.h"
#include <string.h>

/**
 * @brief Parse a BER-TLV DO present inside a buffer of some known length.
 * @param bertlv_parsed Where the parsed BER-TLV will be written.
 * @param bertlv_buf Buffer containing the bytes that are part of the BER-TLV
 * that will be parsed.
 * @param bertlv_buf_len Length of the BER-TLV buffer in order to avoid leaving
 * bounds of the buffer.
 * @note This will only parse the direct children of a DO, to parse nested DOs,
 * a new parser instance would need to be created for it.
 * @return Return code.
 */
__attribute__((unused)) static uicc_ret_et bertlv_parse(
    uicc_bertlv_st *const bertlv_parsed, uint8_t *const bertlv_buf,
    uint32_t const bertlv_buf_len)
{
    return UICC_RET_DO_BERTLV_INVALID;
}

uicc_ret_et uicc_do_bertlv_init(uicc_bertlv_parser_st *const parser,
                                uint8_t *const bertlv_buf,
                                uint32_t const bertlv_buf_len)
{
    memset(parser, 0U, sizeof(uicc_bertlv_parser_st));
    parser->offset = 0U;
    parser->bertlv_buf = bertlv_buf;
    parser->bertlv_buf_len = bertlv_buf_len;
    parser->bertlv_cur_present = false;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_do_bertlv_cur(uicc_bertlv_parser_st *const parser,
                               uicc_bertlv_st *const bertlv_cur)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_do_bertlv_next(uicc_bertlv_parser_st *const parser,
                                uicc_bertlv_st *const bertlv_next)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_do_bertlv_find(uicc_bertlv_parser_st *const parser,
                                uicc_bertlv_st *const bertlv_found)
{
    return UICC_RET_UNKNOWN;
}

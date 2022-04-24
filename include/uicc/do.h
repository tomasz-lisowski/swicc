#pragma once

#include "uicc/common.h"

typedef enum uicc_bertlv_tag_cla_e
{
    UICC_BERTLV_TAG_CLA_INVALID = 0U,
    UICC_BERTLV_TAG_CLA_UNIVERSAL,
    UICC_BERTLV_TAG_CLA_APPLICATION,
    UICC_BERTLV_TAG_CLA_CONTEXT_SPECIFIC,
    UICC_BERTLV_TAG_CLA_PRIVATE,
} uicc_bertlv_tag_cla_et;

typedef enum uicc_bertlv_len_form_e
{
    UICC_BERTLV_LEN_FORM_INVALID = 0U,
    UICC_BERTLV_LEN_FORM_DEFINITE_SHORT,
    UICC_BERTLV_LEN_FORM_DEFINITE_LONG,
    UICC_BERTLV_LEN_FORM_INDEFINITE,
    UICC_BERTLV_LEN_FORM_RESERVED,
} uicc_bertlv_len_form_et;

typedef struct uicc_bertlv_tag_s
{
    uicc_bertlv_tag_cla_et class;
    bool pc; /* False = primitive, True = Constructed. */
    uint32_t tag;
} uicc_bertlv_tag_st;

typedef struct uicc_bertlv_len_s
{
    uicc_bertlv_len_form_et form;
    uint32_t len;
} uicc_bertlv_len_st;

typedef struct uicc_bertlv_s
{
    uicc_bertlv_tag_st tag;
    uicc_bertlv_len_st len;
    uint32_t data_offset;
} uicc_bertlv_st;

typedef struct uicc_bertlv_parser_s
{
    uint32_t bertlv_buf_len;
    uint8_t *bertlv_buf;

    uint32_t offset;
    bool bertlv_cur_present; /* True if the current BER-TLV is present or false
                                if it contains uninitialized data. */
    uicc_bertlv_st bertlv_cur;
} uicc_bertlv_parser_st;

uicc_ret_et uicc_do_bertlv_init(uicc_bertlv_parser_st *const parser,
                                uint8_t *const bertlv_buf,
                                uint32_t const bertlv_buf_len);
uicc_ret_et uicc_do_bertlv_cur(uicc_bertlv_parser_st *const parser,
                               uicc_bertlv_st *const bertlv_cur);
uicc_ret_et uicc_do_bertlv_next(uicc_bertlv_parser_st *const parser,
                                uicc_bertlv_st *const bertlv_next);
uicc_ret_et uicc_do_bertlv_find(uicc_bertlv_parser_st *const parser,
                                uicc_bertlv_st *const bertlv_found);

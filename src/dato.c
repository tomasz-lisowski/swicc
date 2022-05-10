#include "uicc.h"
#include <string.h>

/**
 * @brief Parse a BER-TLV-encoded tag.
 * @param bertlv_tag_prsd Where parsed tag data will be written.
 * @param tag_len Length of the tag will be written here.
 * @param tag_buf Where the raw tag is located.
 * @param tag_buf_size Size of the tag buffer.
 * @return Return code.
 */
static uicc_ret_et bertlv_hdr_tag_prs(
    uicc_dato_bertlv_tag_st *const bertlv_tag_prsd, uint32_t *const tag_len,
    uint8_t const *const tag_buf, uint32_t const tag_buf_size)
{
    *tag_len = 0U;
    uint8_t const tag_b0 = tag_buf[(*tag_len)++];

    /* Tag class. */
    switch ((tag_b0 & 0b11000000) >> 6U)
    {
    case 0:
        bertlv_tag_prsd->cla = UICC_DATO_BERTLV_TAG_CLA_UNIVERSAL;
        break;
    case 1:
        bertlv_tag_prsd->cla = UICC_DATO_BERTLV_TAG_CLA_APPLICATION;
        break;
    case 2:
        bertlv_tag_prsd->cla = UICC_DATO_BERTLV_TAG_CLA_CONTEXT_SPECIFIC;
        break;
    case 3:
        bertlv_tag_prsd->cla = UICC_DATO_BERTLV_TAG_CLA_PRIVATE;
        break;
    default:
        bertlv_tag_prsd->cla = UICC_DATO_BERTLV_TAG_CLA_INVALID;
        break;
    }

    /* P/C. */
    if (tag_b0 & 0b00100000)
    {
        bertlv_tag_prsd->pc = true;
    }
    else
    {
        bertlv_tag_prsd->pc = false;
    }

    /* Tag type. */
    uint8_t const tag_num_b0 = tag_b0 & 0b00011111;
    if (tag_num_b0 == 0b00011111)
    {
        /* Long form. */
        bertlv_tag_prsd->num = 0U;
        for (; *tag_len < UICC_DATO_BERTLV_TAG_LEN_MAX; ++(*tag_len))
        {
            /* Not enough data in the buffer to contain the entire tag. */
            if (*tag_len >= tag_buf_size)
            {
                return UICC_RET_DATO_END;
            }

            /* Safe cast since we iterate for at most 2 times. */
            bertlv_tag_prsd->num |= (uint32_t)((tag_buf[*tag_len] & 0b01111111)
                                               << ((*tag_len - 1U) * 7U));
            /* Bit 8 indicates if there are more tag bytes. */
            if ((tag_buf[*tag_len] & 0b10000000) == 0)
            {
                *tag_len += 1U; /* Include the first byte. */
                return UICC_RET_SUCCESS;
            }
        }
        /**
         * If the for loop iterates completely without returning, it means the
         * tag is too long.
         */
        return UICC_RET_ERROR;
    }
    else
    {
        /* Short form. */
        bertlv_tag_prsd->num = tag_num_b0;
        return UICC_RET_SUCCESS;
    }
}

/**
 * @brief Parse a BER-TLV data object into an internal representation.
 * @param bertlv_prsd Where the parsed BER-TLV DO will be written.
 * @param buf Raw BER-TLV to parse.
 * @param buf_len Length of the buffer containing the raw BER-TLV.
 */
static uicc_ret_et bertlv_hdr_prs(uicc_dato_bertlv_st *const bertlv_prsd,
                                  uint32_t *const bertlv_hdr_len,
                                  uint8_t const *const buf,
                                  uint32_t const buf_len)
{
    if (buf_len < 1)
    {
        return UICC_RET_DATO_END;
    }

    *bertlv_hdr_len = 0U;
    uint32_t tag_len = 0U;
    uicc_ret_et const ret_tag =
        bertlv_hdr_tag_prs(&bertlv_prsd->tag, &tag_len, buf, buf_len);
    if (ret_tag != UICC_RET_SUCCESS)
    {
        return ret_tag;
    }
    *bertlv_hdr_len += tag_len;

    /* Length. */
    uint8_t const len_b0 = buf[(*bertlv_hdr_len)++];
    bertlv_prsd->len.val = len_b0 & 0b01111111;

    /* Determine form. */
    if ((len_b0 & 0b10000000) == 0)
    {
        /* Definite short. */
        bertlv_prsd->len.form = UICC_DATO_BERTLV_LEN_FORM_DEFINITE_SHORT;
    }
    else
    {
        /* Indefinite, definite long, or RFU. */
        switch (len_b0 & 0b01111111)
        {
        case 0:
            /* Not supported. */
            bertlv_prsd->len.form = UICC_DATO_BERTLV_LEN_FORM_INDEFINITE;
            return UICC_RET_ERROR;
        case 127:
            bertlv_prsd->len.form = UICC_DATO_BERTLV_LEN_FORM_RFU;
            return UICC_RET_ERROR;
        default:
            bertlv_prsd->len.form = UICC_DATO_BERTLV_LEN_FORM_DEFINITE_LONG;
            /**
             * Number of bytes after b0 of len that are part of the length
             * field.
             */
            uint8_t len_len = len_b0 & 0b01111111;

            if (len_len > UICC_DATO_BERTLV_LEN_LEN_MAX)
            {
                return UICC_RET_ERROR;
            }

            for (uint8_t len_idx = 1U; len_idx < len_len; ++len_idx)
            {
                if (buf_len < *bertlv_hdr_len)
                {
                    return UICC_RET_DATO_END;
                }
                /* Safe cast since we iterate for at most 2 times. */
                bertlv_prsd->len.val |= (uint32_t)(buf[(*bertlv_hdr_len)++]
                                                   << ((len_idx - 1U) * 7U));
            }
            break;
        }
    }

    return UICC_RET_SUCCESS;
}

/**
 * @brief Convert the internal representation of a BER-TLV into the serialized
 * format. This populates the given buffer back from the end towards the front.
 * @param bertlv_deprs The BER-TLV to deparse.
 * @param buf Where to write the BER-TLV in the serialized format. If this is
 * NULL, the length will still be computed but the encoded data will not be
 * written anywhere.
 * @param len Must contain the maximum buffer length. It receives the actual
 * length written to the buffer. Can be anything when buffer is NULL.
 * @return Return code.
 */
static uicc_ret_et bertlv_hdr_deprs(
    uicc_dato_bertlv_st const *const bertlv_deprs, uint8_t *const buf,
    uint32_t *const len)
{
    bool const dry_run = buf == NULL;

    /* The provided BER-TLV is invalid. */
    if (bertlv_deprs->len.form == UICC_DATO_BERTLV_LEN_FORM_INVALID ||
        bertlv_deprs->tag.cla == UICC_DATO_BERTLV_TAG_CLA_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }

    uint32_t const buf_len = *len;
    *len = 0U;

    uint8_t len_raw[UICC_DATO_BERTLV_LEN_LEN_MAX] = {0};
    uint8_t len_len = 0U;
    len_raw[len_len] = 0U;
    {
        if (bertlv_deprs->len.form == UICC_DATO_BERTLV_LEN_FORM_DEFINITE_SHORT)
        {
            /* Safe cast since short length forms are 7 bits wide. */
            len_raw[len_len] = (uint8_t)bertlv_deprs->len.val;
            len_len = 1U;

            /* The most significant bit must be 0 in short form. */
            if (len_raw[len_len] & 0b10000000)
            {
                return UICC_RET_ERROR;
            }
        }
        else
        {
            /* For now skip the first byte of the length. */
            len_len = 1U;

            uint8_t zero_cnt = 0U;

            for (; len_len < UICC_DATO_BERTLV_LEN_LEN_MAX; ++len_len)
            {
                /**
                 * Safe cast because the expression extracts exactly one byte.
                 */
                len_raw[len_len] =
                    (uint8_t)((bertlv_deprs->len.val >> (8U * (len_len - 1U))) &
                              0xFF);

                /**
                 * Keep track of the number of trailing zeros at the end of the
                 * length so they can be trimmed from the final length field.
                 */
                if (len_raw[len_len] == 0)
                {
                    /* Safe cast due to range of loop. */
                    zero_cnt = (uint8_t)(zero_cnt + 1);
                }
                else
                {
                    zero_cnt = 0;
                }
            }
            /* Safe cast, this will never be negative. */
            len_len = (uint8_t)(len_len - zero_cnt);

            /* Safe cast since this will only form a byte. */
            len_raw[0U] = (uint8_t)(0b10000000 | (len_len - 1U));
        }

        if (!dry_run)
        {
            /* Check if the length fits in the buffer. */
            if (len_len > buf_len - *len)
            {
                return UICC_RET_BUFFER_TOO_SHORT;
            }
        }

        *len += len_len;
        if (!dry_run)
        {
            memcpy(&buf[buf_len - *len], len_raw, len_len);
        }
    }

    uint8_t tag_raw[UICC_DATO_BERTLV_TAG_LEN_MAX] = {0};
    uint8_t tag_len = 0U;
    {
        switch (bertlv_deprs->tag.cla)
        {
        case UICC_DATO_BERTLV_TAG_CLA_UNIVERSAL:
            /* Just 0b00 so no need to write any bits. */
            break;
        case UICC_DATO_BERTLV_TAG_CLA_APPLICATION:
            tag_raw[0U] |= 0b01000000;
            break;
        case UICC_DATO_BERTLV_TAG_CLA_CONTEXT_SPECIFIC:
            tag_raw[0U] |= 0b10000000;
            break;
        case UICC_DATO_BERTLV_TAG_CLA_PRIVATE:
            tag_raw[0U] |= 0b11000000;
            break;
        default:
            return UICC_RET_ERROR;
        }

        if (bertlv_deprs->tag.pc)
        {
            tag_raw[0U] |= 0b00100000;
        } /* bit 3 is 0 for primitive BER-TLVs so no need to edit anything. */

        /* If >30 then it's a long tag. */
        if (bertlv_deprs->tag.num > 30)
        {
            /* Must be all 1's to indicate a long tag. */
            tag_raw[0U] |= 0b00011111;
            tag_len = 1U; /* There is at least 1 byte in the tag. */

            uint8_t zero_cnt = 0U;

            for (; tag_len < UICC_DATO_BERTLV_TAG_LEN_MAX; ++tag_len)
            {
                /**
                 * Safe cast because the expression extracts exactly one byte (7
                 * bits to be exact).
                 */
                tag_raw[tag_len] =
                    (uint8_t)((bertlv_deprs->tag.num >> (7U * (tag_len - 1U))) &
                              0x7F);

                /**
                 * Keep track of the number of trailing zeros at the end of the
                 * tag so they can be trimmed from the final tag field.
                 */
                if (tag_raw[tag_len] == 0)
                {
                    /* Safe cast due to range of loop. */
                    zero_cnt = (uint8_t)(zero_cnt + 1);
                }
                else
                {
                    zero_cnt = 0;
                }
            }
            /* Safe cast, this will never be negative. */
            tag_len = (uint8_t)(tag_len - zero_cnt);
        }
        else
        {
            /* Safe cast since the tag number is expected */
            tag_raw[0U] |= (uint8_t)bertlv_deprs->tag.num;
            tag_len = 1U;
        }

        if (!dry_run)
        {
            /* Check if the tag fits in the buffer. */
            if (tag_len > buf_len - *len)
            {
                return UICC_RET_BUFFER_TOO_SHORT;
            }
        }

        *len += tag_len;
        if (!dry_run)
        {
            memcpy(&buf[buf_len - *len], tag_raw, tag_len);
        }
    }

    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_dato_bertlv_tag_create(
    uicc_dato_bertlv_tag_st *const bertlv_tag_out, uint32_t const tag)
{
    uint8_t *const tag_buf = (uint8_t *)&tag;
    uint32_t tag_len = 0U;
    return bertlv_hdr_tag_prs(bertlv_tag_out, &tag_len, tag_buf, sizeof(tag));
}

void uicc_dato_bertlv_dec_init(uicc_dato_bertlv_dec_st *const decoder,
                               uint8_t *const bertlv_buf,
                               uint32_t const bertlv_len)
{
    memset(decoder, 0U, sizeof(*decoder));
    decoder->buf = bertlv_buf;
    decoder->len = bertlv_len;
    decoder->offset = 0U;
    decoder->cur.tag.cla = UICC_DATO_BERTLV_TAG_CLA_INVALID;
    decoder->cur.len.form = UICC_DATO_BERTLV_LEN_FORM_INVALID;
}

uicc_ret_et uicc_dato_bertlv_dec_cur(uicc_dato_bertlv_dec_st *const decoder,
                                     uicc_dato_bertlv_dec_st *const decoder_cur,
                                     uicc_dato_bertlv_st *const bertlv_cur)
{
    /* Never called next so there is no current BER-TLV DO to get. */
    if (decoder->offset == 0U)
    {
        return UICC_RET_ERROR;
    }
    uint32_t const offset_cur_val = decoder->offset - decoder->cur.len.val;
    *bertlv_cur = decoder->cur;
    uicc_dato_bertlv_dec_init(decoder_cur, &decoder->buf[offset_cur_val],
                              decoder->cur.len.val);
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_dato_bertlv_dec_next(uicc_dato_bertlv_dec_st *const decoder)
{
    if (decoder->offset >= decoder->len)
    {
        /* No more data to parse. */
        return UICC_RET_DATO_END;
    }
    uicc_ret_et ret = bertlv_hdr_prs(&decoder->cur, &decoder->cur_len_hdr,
                                     &decoder->buf[decoder->offset],
                                     decoder->len - decoder->offset);
    if (ret == UICC_RET_SUCCESS)
    {
        /* Move offset to after the end of the current BER-TLV DO. */
        decoder->offset += decoder->cur_len_hdr + decoder->cur.len.val;
    }
    return ret;
}

void uicc_dato_bertlv_enc_init(uicc_dato_bertlv_enc_st *const encoder,
                               uint8_t *const buf, uint32_t const buf_size)
{
    bool const dry_run = buf == NULL;
    memset(encoder, 0U, sizeof(*encoder));
    encoder->buf = buf;
    if (dry_run)
    {
        encoder->size = UINT32_MAX;
    }
    else
    {
        encoder->size = buf_size;
    }
    encoder->len = 0U;
    encoder->offset = encoder->size;
    encoder->len_val = 0U;
}

uicc_ret_et uicc_dato_bertlv_enc_nstd_start(
    uicc_dato_bertlv_enc_st *const encoder,
    uicc_dato_bertlv_enc_st *const encoder_nstd)
{
    /* Can't begin nesting when there is some data without a header. */
    if (encoder->len_val != 0)
    {
        return UICC_RET_ERROR;
    }
    uicc_dato_bertlv_enc_init(encoder_nstd, encoder->buf,
                              encoder->size - encoder->len);
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_dato_bertlv_enc_nstd_end(
    uicc_dato_bertlv_enc_st *const encoder,
    uicc_dato_bertlv_enc_st *const encoder_nstd)
{
    /* Can't end nesting with data without a header inside the nested block. */
    if (encoder->len_val != 0)
    {
        return UICC_RET_ERROR;
    }
    encoder->len += encoder_nstd->len;
    encoder->len_val = encoder_nstd->len;
    encoder->offset -= encoder_nstd->len;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_dato_bertlv_enc_hdr(uicc_dato_bertlv_enc_st *const encoder,
                                     uicc_dato_bertlv_tag_st const *const tag)
{
    uint32_t const len = encoder->len_val;
    uicc_dato_bertlv_len_form_et len_form;
    if (len <= 127U)
    {
        len_form = UICC_DATO_BERTLV_LEN_FORM_DEFINITE_SHORT;
    }
    else if (len <= UINT32_MAX)
    {
        len_form = UICC_DATO_BERTLV_LEN_FORM_DEFINITE_LONG;
    }
    else
    {
        /* Value length can't exceed what can be stored in a 4 byte uint. */
        return UICC_RET_ERROR;
    }
    uicc_dato_bertlv_st const bertlv = {.tag = *tag,
                                        .len = {.val = len, .form = len_form}};
    uint32_t len_hdr = encoder->size - encoder->len;
    uicc_ret_et const ret_deprs =
        bertlv_hdr_deprs(&bertlv, encoder->buf, &len_hdr);
    if (ret_deprs == UICC_RET_SUCCESS)
    {
        encoder->len += len_hdr;
        encoder->offset -= len_hdr;
        encoder->len_val = 0U;
    }
    return ret_deprs;
}

uicc_ret_et uicc_dato_bertlv_enc_data(uicc_dato_bertlv_enc_st *const encoder,
                                      uint8_t const *const data,
                                      uint32_t const data_len)
{
    bool const dry_run = encoder->buf == NULL;

    /* Cast to force a signed comparison. */
    if (encoder->offset - (int64_t)data_len < 0)
    {
        /* Not enough space for the data. */
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    encoder->offset -= data_len;
    encoder->len_val += data_len;
    encoder->len += data_len;
    if (!dry_run)
    {
        memcpy(&encoder->buf[encoder->offset], data, data_len);
    }
    return UICC_RET_SUCCESS;
}

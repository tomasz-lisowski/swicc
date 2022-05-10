#pragma once
/**
 * DATO = Data Object. In standards just DO but 'do' is reserved in C so 'dato'
 * is used to avoid problems.
 */

#include "uicc/common.h"
#include <assert.h>

/**
 * The limit on the number of bytes in the tag (=3) is mentioned in ISO
 * 7816-4:2020 p.20 sec.6.3.
 */
#define UICC_DATO_BERTLV_TAG_LEN_MAX 3U

/**
 * ISO 7816-4:2020 p.20 sec.6.3 states that length fields longer than 5 bytes
 * (including the initial byte b0) are RFU.
 */
#define UICC_DATO_BERTLV_LEN_LEN_MAX 5U

typedef enum uicc_dato_bertlv_tag_cla_e
{
    UICC_DATO_BERTLV_TAG_CLA_INVALID,
    UICC_DATO_BERTLV_TAG_CLA_UNIVERSAL,
    UICC_DATO_BERTLV_TAG_CLA_APPLICATION,
    UICC_DATO_BERTLV_TAG_CLA_CONTEXT_SPECIFIC,
    UICC_DATO_BERTLV_TAG_CLA_PRIVATE,
} uicc_dato_bertlv_tag_cla_et;

typedef enum uicc_dato_bertlv_len_form_e
{
    UICC_DATO_BERTLV_LEN_FORM_INVALID,
    UICC_DATO_BERTLV_LEN_FORM_DEFINITE_SHORT,
    UICC_DATO_BERTLV_LEN_FORM_DEFINITE_LONG,
    UICC_DATO_BERTLV_LEN_FORM_INDEFINITE, /* Unsupported per ISO 7816-4:2020
                                             p.20 sec.6.3. */
    UICC_DATO_BERTLV_LEN_FORM_RFU,
} uicc_dato_bertlv_len_form_et;

typedef struct uicc_dato_bertlv_tag_s
{
    uint32_t num;
    uicc_dato_bertlv_tag_cla_et cla;
    bool pc; /* False = primitive, True = constructed. */
} uicc_dato_bertlv_tag_st;

typedef struct uicc_dato_bertlv_len_s
{
    uint32_t val;
    uicc_dato_bertlv_len_form_et form;
} uicc_dato_bertlv_len_st;

/**
 * -1 because one byte is used to indicate the number of octets to come, so
 * those bits are not part of the length value.
 */
static_assert(UICC_DATO_BERTLV_LEN_LEN_MAX - 1U ==
                  sizeof((uicc_dato_bertlv_len_st){0}.val),
              "The maximum BER-TLV len field length does not match the size of "
              "the value data type used to store it");

typedef struct uicc_dato_bertlv_s
{
    uicc_dato_bertlv_tag_st tag;
    uicc_dato_bertlv_len_st len;
} uicc_dato_bertlv_st;

typedef struct uicc_dato_bertlv_dec_s
{
    uint8_t *buf;
    uint32_t len;

    uint32_t offset;
    uint32_t cur_len_hdr;
    uicc_dato_bertlv_st cur;
} uicc_dato_bertlv_dec_st;

typedef struct uicc_dato_bertlv_enc_s
{
    uint8_t *buf;
    uint32_t len;  /* Occupied size of buffer.  */
    uint32_t size; /* Allocated size of buffer. */

    uint32_t
        len_val; /* Length of the value of the actively encoded BER-TLV DO. */
    uint32_t offset;
} uicc_dato_bertlv_enc_st;

/**
 * @brief A helper for creating a BER-TLV tag struct from a tag number.
 * @param bertlv_tag_out Where to store the created BER-TLV tag.
 * @param tag Tag of the BER-TLV. This is NOT the tag number, rather it is the
 * raw tag as described in e.g. ISO 7816-4:2020 p.27 sec.7.4.3 table.11.
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_tag_create(
    uicc_dato_bertlv_tag_st *const bertlv_tag_out, uint32_t const tag);

/**
 * @brief Initialize decoding operation on a buffer containing an encoded
 * BER-TLV DO.
 * @param decoder The decoder struct to populate (initialize).
 * @param bertlv_buf Buffer containing the BER-TLV encoded DO.
 * @param bertlv_len Length of the BER-TLV DO in bytes.
 */
void uicc_dato_bertlv_dec_init(uicc_dato_bertlv_dec_st *const decoder,
                               uint8_t *const bertlv_buf,
                               uint32_t const bertlv_len);

/**
 * @brief Retrieves the item at the head of the decoder.
 * @param decoder
 * @param decoder_cur A decoder for the current BER-TLV that is returned.
 * @param bertlv_cur Where to write the current BER-TLV item.
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_dec_cur(uicc_dato_bertlv_dec_st *const decoder,
                                     uicc_dato_bertlv_dec_st *const decoder_cur,
                                     uicc_dato_bertlv_st *const bertlv_cur);

/**
 * @brief Move the decoder head to the next item in the BER-TLV DO then parse
 * this item.
 * @param decoder
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_dec_next(uicc_dato_bertlv_dec_st *const decoder);

/**
 * @brief Initialize a BRT-TLV encoder in preparation for encoding a BER-TLV DO.
 * @param encoder
 * @param buf Buffer that will receive the encoded BER-TLV string.
 * @param buf_size Size of the provided buffer.
 * @note Encoding is done backwards i.e. from data of the last DO to the header
 * of the first DO.
 */
void uicc_dato_bertlv_enc_init(uicc_dato_bertlv_enc_st *const encoder,
                               uint8_t *const buf, uint32_t const buf_size);

/**
 * @brief Begin encoding a nested item.
 * @param encoder The encoder for the parent.
 * @param encoder_nstd The encoder for the nested item.
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_enc_nstd_start(
    uicc_dato_bertlv_enc_st *const encoder,
    uicc_dato_bertlv_enc_st *const encoder_nstd);

/**
 * @brief End encoding of a nested item.
 * @param encoder The encoder for the parent.
 * @param encoder_nstd The encoder for the nested item.
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_enc_nstd_end(
    uicc_dato_bertlv_enc_st *const encoder,
    uicc_dato_bertlv_enc_st *const encoder_nstd);

/**
 * @brief Encode the header of a BER-TLV DO.
 * @param encoder
 * @param tag Only the tag is required here because the length is computed based
 * on previous call to encode.
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_enc_hdr(uicc_dato_bertlv_enc_st *const encoder,
                                     uicc_dato_bertlv_tag_st const *const tag);

/**
 * @brief Write the provided data into the encoded buffer and keep track of its
 * length for when encoding the header.
 * @param encoder
 * @param data
 * @param data_len
 * @return Return code.
 */
uicc_ret_et uicc_dato_bertlv_enc_data(uicc_dato_bertlv_enc_st *const encoder,
                                      uint8_t const *const data,
                                      uint32_t const data_len);

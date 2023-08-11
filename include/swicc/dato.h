#pragma once
/**
 * DATO = Data Object. In standards just DO but 'do' is reserved in C so 'dato'
 * is used to avoid problems.
 *
 * @note BER-TLV encoding is done in reverse! It is more efficient to do it
 * this way, no memory moving, just writes. For example:
 *
 * ENCODER NESTED 0 START
 *   ENCODE DATA3
 *   ENCODE DATA2
 *   ENCODE HEADER T3
 *   ENCODER NESTED 1 START
 *     ENCODE DATA1
 *     ENCODE HEADER T2
 *   ENCODER NESTED 1 END
 *   ENCODE HEADER T1
 * ENCODER NESTED 0 END
 * ENCODE HEADER T0
 *
 * The above encodes:
 * {T0-L0-
 *   {T1-L1-
 *     {T2-L2-DATA1}
 *   }
 *   {T3-L3-DATA2-DATA3}
 * }
 *
 * @note The length fields are computed automatically.
 *
 * @note At the root, there must be exactly one DO i.e. {T-L-V}{T-L-V}... is not
 * supported but inside constructed DOs this is possible. In summary one
 * possible data object is: {T-L-{T-L-{T-L-V}}{T-L-V}}
 *
 * @note The decoder for BER-TLV does not recurse into nested objects. The
 * caller is given all information necessary to implement recursion themselves.
 */

#include "swicc/common.h"
#include <assert.h>

/**
 * The limit on the number of bytes in the tag (=3) is mentioned in ISO/IEC
 * 7816-4:2020 p.20 sec.6.3.
 */
#define SWICC_DATO_BERTLV_TAG_LEN_MAX 3U /* In bytes. */

/**
 * ISO/IEC 7816-4:2020 p.20 sec.6.3 states that length fields longer than 5
 * bytes (including the initial byte b0) are RFU.
 */
#define SWICC_DATO_BERTLV_LEN_LEN_MAX 5U /* In bytes. */

/* Tag class. */
typedef enum swicc_dato_bertlv_tag_cla_e
{
    SWICC_DATO_BERTLV_TAG_CLA_INVALID,
    SWICC_DATO_BERTLV_TAG_CLA_UNIVERSAL,
    SWICC_DATO_BERTLV_TAG_CLA_APPLICATION,
    SWICC_DATO_BERTLV_TAG_CLA_CONTEXT_SPECIFIC,
    SWICC_DATO_BERTLV_TAG_CLA_PRIVATE,
} swicc_dato_bertlv_tag_cla_et;

/* Type of length field. */
typedef enum swicc_dato_bertlv_len_form_e
{
    SWICC_DATO_BERTLV_LEN_FORM_INVALID,
    SWICC_DATO_BERTLV_LEN_FORM_DEFINITE_SHORT,
    SWICC_DATO_BERTLV_LEN_FORM_DEFINITE_LONG,
    SWICC_DATO_BERTLV_LEN_FORM_INDEFINITE, /* Unsupported per ISO/IEC
                                              7816-4:2020 p.20 sec.6.3. */
    SWICC_DATO_BERTLV_LEN_FORM_RFU,
} swicc_dato_bertlv_len_form_et;

typedef struct swicc_dato_bertlv_tag_s
{
    uint32_t num;
    swicc_dato_bertlv_tag_cla_et cla;
    bool pc; /* False = primitive, True = constructed. */
} swicc_dato_bertlv_tag_st;

typedef struct swicc_dato_bertlv_len_s
{
    uint32_t val;
    swicc_dato_bertlv_len_form_et form;
} swicc_dato_bertlv_len_st;

/**
 * -1 because one byte is used to indicate the number of octets to come, so
 * those bits are not part of the length value.
 */
static_assert(
    SWICC_DATO_BERTLV_LEN_LEN_MAX - 1U ==
                  sizeof((swicc_dato_bertlv_len_st){0}.val),
    "The maximum BER-TLV len field length does not match the size of the value data type used to store it.");

typedef struct swicc_dato_bertlv_s
{
    swicc_dato_bertlv_tag_st tag;
    swicc_dato_bertlv_len_st len;
} swicc_dato_bertlv_st;

typedef struct swicc_dato_bertlv_dec_s
{
    uint8_t *buf;
    uint32_t len;

    uint32_t offset;
    uint32_t cur_len_hdr;
    swicc_dato_bertlv_st cur;
} swicc_dato_bertlv_dec_st;

typedef struct swicc_dato_bertlv_enc_s
{
    uint8_t *buf;
    uint32_t len;  /* Occupied size of buffer.  */
    uint32_t size; /* Allocated size of buffer. */

    uint32_t
        len_val; /* Length of the value of the actively encoded BER-TLV DO. */
    uint32_t offset;
} swicc_dato_bertlv_enc_st;

/**
 * @brief A helper for creating a BER-TLV tag struct from a tag number (the tag
 * byte(s)).
 * @param[out] bertlv_tag_out Where to store the created BER-TLV tag.
 * @param[in] tag Tag of the BER-TLV. This is NOT the tag number, rather it is
 * the raw tag (bytes) as described in e.g. ISO/IEC 7816-4:2020 p.27 sec.7.4.3
 * table.11.
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_tag_create(
    swicc_dato_bertlv_tag_st *const bertlv_tag_out, uint32_t const tag);

/**
 * @brief Initialize decoding operation on a buffer containing an encoded
 * BER-TLV DO.
 * @param[out] decoder The decoder struct to populate (initialize).
 * @param[in] bertlv_buf Buffer containing the BER-TLV encoded DO.
 * @param[in] bertlv_len Length of the BER-TLV DO in bytes.
 */
void swicc_dato_bertlv_dec_init(swicc_dato_bertlv_dec_st *const decoder,
                                uint8_t *const bertlv_buf,
                                uint32_t const bertlv_len);

/**
 * @brief Retrieves the item at the head of the decoder.
 * @param[in, out] decoder
 * @param[out] decoder_cur A decoder for the current BER-TLV that is returned.
 * @param[out] bertlv_cur Where to write the current BER-TLV item.
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_dec_cur(
    swicc_dato_bertlv_dec_st *const decoder,
    swicc_dato_bertlv_dec_st *const decoder_cur,
    swicc_dato_bertlv_st *const bertlv_cur);

/**
 * @brief Move the decoder head to the next item in the BER-TLV DO then parse
 * this item.
 * @param[in, out] decoder
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_dec_next(
    swicc_dato_bertlv_dec_st *const decoder);

/**
 * @brief Initialize a BRT-TLV encoder in preparation for encoding a BER-TLV DO.
 * @param[out] encoder
 * @param[in] buf Buffer that will receive the encoded BER-TLV string. If this
 * is NULL, the encoding will be considered a dry-run which is useful for
 * getting the total length of the encoded BER-TLV DO without writing to the
 * destinaton memory.
 * @param[in] buf_size Size of the provided buffer.
 * @note Encoding is done backwards i.e. from data of the last DO to the header
 * of the first DO. This can be anything when the buffer is NULL.
 */
void swicc_dato_bertlv_enc_init(swicc_dato_bertlv_enc_st *const encoder,
                                uint8_t *const buf, uint32_t const buf_size);

/**
 * @brief Begin encoding of a nested item.
 * @param[in, out] encoder The encoder for the parent.
 * @param[in, out] encoder_nstd The encoder for the nested item.
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_enc_nstd_start(
    swicc_dato_bertlv_enc_st *const encoder,
    swicc_dato_bertlv_enc_st *const encoder_nstd);

/**
 * @brief End encoding of a nested item.
 * @param[in, out] encoder The encoder for the parent.
 * @param[in, out] encoder_nstd The encoder for the nested item.
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_enc_nstd_end(
    swicc_dato_bertlv_enc_st *const encoder,
    swicc_dato_bertlv_enc_st *const encoder_nstd);

/**
 * @brief Encode the header of a BER-TLV DO.
 * @param[in, out] encoder
 * @param[in] tag Only the tag is required here because the length is computed
 * based on previous call(s) to encode.
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_enc_hdr(
    swicc_dato_bertlv_enc_st *const encoder,
    swicc_dato_bertlv_tag_st const *const tag);

/**
 * @brief Write the provided data into the encoded buffer and keep track of its
 * length for when encoding the header.
 * @param[in, out] encoder
 * @param[in] data
 * @param[in] data_len
 * @return Return code.
 */
swicc_ret_et swicc_dato_bertlv_enc_data(swicc_dato_bertlv_enc_st *const encoder,
                                        uint8_t const *const data,
                                        uint32_t const data_len);

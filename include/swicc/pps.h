#pragma once

#include "swicc/common.h"

/**
 * PPS = protocol and parameter selection.
 */

/**
 * According to ISO/IEC 7816-3:2006 clause.9.1, 0xFF shall be the first
 * byte of every PPS request and response.
 */
#define SWICC_PPS_PPSS ((uint8_t)0xFF)

/**
 * ISO/IEC 7816-3:2006 clause.9.2 shows a diagram where the possible PPS
 * bytes are PPSS, PPS0, PPS1, PPS2, PPS3, PCK so 6 in total.
 */
#define SWICC_PPS_LEN_MAX 6U

/**
 * Represents the parameters that result from a PPS negotiation. Note that the
 * params are not stored as their actual values, rather as indices that shall be
 * resolved to values later, using an appropriate lookup table for each (this
 * table is in the I/O module).
 */
typedef struct swicc_pps_params_s
{
    uint8_t t; /* The transmission protocol type */
    uint8_t fi_idx;
    /* f(max) is derived from Fi */
    uint8_t di_idx;
    uint8_t spu;
} swicc_pps_params_st;

/**
 * @brief Handles a PPS request coming from an interface device and forms a PPS
 * response.
 * @param[out] pps_params Where to write the negotiated params. Only valid on
 * success.
 * @param[in] buf_rx Received PPS request.
 * @param[in] buf_rx_len Length of received PPS request.
 * @param[out] buf_tx Where the PPS response will be written.
 * @param[in, out] buf_tx_len Must contain the size of the TX buffer. It will
 * get the PPS response length.
 * @return Return code.
 */
swicc_ret_et swicc_pps(swicc_pps_params_st *const pps_params,
                       uint8_t const *const buf_rx, uint16_t const buf_rx_len,
                       uint8_t *const buf_tx, uint16_t *const buf_tx_len);

/**
 * @brief Compute the expected length of the PPS message.
 * @param[in] pps A complete or partial PPS message. This must be at least 2
 * bytes and the first byte must be a PPSS for this function to work.
 * @param[in] pps_len Length of the given PPS.
 * @param[out] pps_len_exp Where the expected PPS length will be written.
 * @return Return code.
 */
swicc_ret_et swicc_pps_len(uint8_t const *const pps, uint8_t const pps_len,
                           uint8_t *const pps_len_exp);

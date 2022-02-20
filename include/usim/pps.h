#include <stdint.h>
#include <usim/common.h>

#pragma once
/**
 * PPS = protocol and parameter selection
 */

/**
 * According to ISO 7816-3:2006 p.20 sec.9.1, 0xFF shall be the first byte of
 * every PPS request and response.
 */
#define USIM_PPS_PPSS ((uint8_t)0xFF)

/**
 * ISO 7816-3:2006 p.21 sec.9.2 shows a diagram where the possible PPS bytes are
 * PPSS, PPS0, PPS1, PPS2, PPS3, PCK so 6 in total.
 */
#define USIM_PPS_LEN_MAX 6U

/**
 * Represents the parameters that result from a PPS negotiation. Note that the
 * params are not stored as their actual values, rather as indices that shall be
 * resolved to values later, using an appropriate lookup table for each.
 */
typedef struct pps_params_s
{
    uint8_t t; /* The transmission protocol type */
    uint8_t fi_idx;
    /* f(max) is derived from Fi */
    uint8_t di_idx;
    uint8_t spu;
} pps_params_st;

/**
 * @brief Handles a PPS request coming from an interface device and forms a PPS
 * response.
 * @param pps_params Where to write the negotiated params. Only valid on
 * success.
 * @param buf_rx Received PPS request.
 * @param buf_rx_len Length of received PPS request.
 * @param buf_tx Where the PPS response will be written.
 * @param buf_tx_len Where the PPS response length will be written.
 * @return Return code.
 */
usim_ret_et usim_pps(pps_params_st *const pps_params,
                     uint8_t const *const buf_rx, uint16_t const buf_rx_len,
                     uint8_t *const buf_tx, uint16_t *const buf_tx_len);

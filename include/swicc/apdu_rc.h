#pragma once
/**
 * Manages the buffer used in response chaining. This means that APDU
 * instruction handlers can give data to this buffer for later retrieval (in
 * parts or in one go) by the interface through use of the GET RESPONSE
 * instruction.
 */

#include "swicc/common.h"

/**
 * Contains all data for managing and storing the response chaining (RC) buffer.
 */
typedef struct swicc_apdu_rc_s
{
    uint8_t b[SWICC_DATA_MAX];
    uint32_t len;

    /* How much of the data was already returned to the interface. */
    uint32_t offset;
} swicc_apdu_rc_st;

/**
 * @brief Reset the response chaining buffer.
 * @param[in, out] rc
 * @warning ISO/IEC 7816-4:2020 p.12 sec.5.3.4 states that the behavior of the
 * card, if the interface tries to resume response chaining after another
 * command is run in between GET RESPONSE instructions, is undefined. By
 * resetting the buffer at the start of instructions, the behavior can be made
 * deterministic i.e. resuming response chaining would always fail.
 */
void swicc_apdu_rc_reset(swicc_apdu_rc_st *const rc);

/**
 * @brief Enqueue data in the RC buffer.
 * @param[in, out] rc
 * @param[in] buf Shall contain the data to enqueue.
 * @param[in] buf_len Shall contain the length of the buffer.
 * @return Return code.
 */
swicc_ret_et swicc_apdu_rc_enq(swicc_apdu_rc_st *const rc,
                               uint8_t const *const buf,
                               uint32_t const buf_len);

/**
 * @brief Dequeue data from the RC buffer.
 * @param[in, out] rc
 * @param[out] buf Buffer to write the dequeued data into.
 * @param[in, out] buf_len Shall contain the size of the given buffer (or if
 * trying to dequeue less data, set this to the requested amount). It will
 * receive the dequeued data length on success.
 * @return Return code.
 * @note If more data was requested than was available, the function will fail
 * and store the length of available data in the buffer length parameter.
 */
swicc_ret_et swicc_apdu_rc_deq(swicc_apdu_rc_st *const rc, uint8_t *const buf,
                               uint32_t *const buf_len);

/**
 * @brief Return how much data is left in the RC buffer.
 * @param[in] rc
 * @return Number of bytes left in the RC buffer.
 */
uint32_t swicc_apdu_rc_len_rem(swicc_apdu_rc_st const *const rc);

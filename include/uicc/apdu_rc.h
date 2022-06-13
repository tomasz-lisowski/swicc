#pragma once
/**
 * Manages the buffer used in response chaining. This means that APDU
 * instruction handlers can give data to this buffer for later retrieval (in
 * parts or in one go) by the interface through use of the GET RESPONSE
 * instruction.
 */

#include "uicc/common.h"

/**
 * Contains all data for managing and storing the response chaining (RC) buffer.
 */
typedef struct uicc_apdu_rc_s
{
    uint8_t b[UICC_DATA_MAX];
    uint32_t len;

    /* How much of the data was already returned to the interface. */
    uint32_t offset;
} uicc_apdu_rc_st;

/**
 * @brief Reset the response chaining buffer.
 * @warning ISO 7816-4:2020 p.12 sec.5.3.4 states that the behavior of the card,
 * if the interface tries to resume response chaining after another command is
 * run in between GET RESPONSE instructions, is undefined. By resetting the
 * buffer at the start of instructions, the behavior can be made deterministic
 * i.e. resuming response chaining would always fail.
 * @param rc
 */
void uicc_apdu_rc_reset(uicc_apdu_rc_st *const rc);

/**
 * @brief Enqueue data in the RC buffer.
 * @param rc
 * @param buf Shall contain the data to enqueue.
 * @param buf_len Shall contain the length of the buffer.
 * @return Return code.
 */
uicc_ret_et uicc_apdu_rc_enq(uicc_apdu_rc_st *const rc,
                             uint8_t const *const buf, uint32_t const buf_len);

/**
 * @brief Dequeue data from the RC buffer.
 * @param buf Buffer to write the dequeued data into.
 * @param buf_len Shall contain the size of the given buffer (or if trying to
 * dequeue less data, set this to the requested amount). It will receive the
 * dequeued data length on success.
 * @return Return code.
 * @note If more data was requested than was available, the function will fail
 * and store the length of available data in the buffer length parameter.
 */
uicc_ret_et uicc_apdu_rc_deq(uicc_apdu_rc_st *const rc, uint8_t *const buf,
                             uint32_t *const buf_len);

/**
 * @brief Return how much data is left in the RC buffer.
 * @param rc
 * @return Number of bytes left in the RC buffer.
 */
uint32_t uicc_apdu_rc_len_rem(uicc_apdu_rc_st *const rc);
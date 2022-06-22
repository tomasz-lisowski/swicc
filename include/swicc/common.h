#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SWICC_DATA_MAX_SHRT 256U
#define SWICC_DATA_MAX_LONG 65536U
#define SWICC_DATA_MAX SWICC_DATA_MAX_SHRT

/**
 * All possible return codes that can get returned from the functions of this
 * library.
 */
typedef enum swicc_ret_e
{
    SWICC_RET_UNKNOWN = 0,
    SWICC_RET_SUCCESS =
        1,           /* In principle =1, allows for use as 'if' condition. */
    SWICC_RET_ERROR, /* Unspecified error (non-critical). */
    SWICC_RET_PARAM_BAD, /* Generic error to indicate the parameter was bad. */

    SWICC_RET_APDU_HDR_TOO_SHORT,
    SWICC_RET_APDU_UNHANDLED,

    SWICC_RET_APDU_RES_INVALID,
    SWICC_RET_TPDU_HDR_TOO_SHORT,
    SWICC_RET_BUFFER_TOO_SHORT,

    SWICC_RET_PPS_INVALID, /* E.g. the check byte is incorrect etc... */
    SWICC_RET_PPS_FAILED,  /* Request is handled but params are not accepted */

    SWICC_RET_ATR_INVALID,  /* E.g. the ATR might not contain madatory fields or
                              is malformed. */
    SWICC_RET_FS_NOT_FOUND, /* Requested FS item is not present. */

    SWICC_RET_DATO_END, /* Reached end of buffer/data. */

    SWICC_RET_NET_CONN_QUEUE_EMPTY, /* (Client) Connection queue is empty i.e.
                                       there are no pending connections to the
                                       server. */
} swicc_ret_et;

/**
 * Typedef these to avoid including and creating circular deps.
 */
typedef struct swicc_s swicc_st;
typedef struct swicc_fs_file_s swicc_fs_file_st;
typedef enum swicc_fsm_state_e swicc_fsm_state_et;
typedef struct swicc_net_msg_s swicc_net_msg_st;

/**
 * @brief Compute the elementary time unit (ETU) as described in ISO 7816-3:2006
 * p.13 sec.7.1.
 * @param etu Where the computed ETU will be written.
 * @param fi The clock rate conversion integer (Fi).
 * @param di The baud rate adjustment integer (Di).
 * @param fmax The maximum supported clock frequency (f(max)).
 */
void swicc_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
               uint32_t const fmax);

/**
 * @brief Compute check byte for a buffer. This means the result of XOR'ing all
 * bytes together. ISO 7816-3:2006 p.18 sec.8.2.5.
 * @param buf_raw Buffer.
 * @param buf_raw_len Length of the data in the buffer.
 * @return XOR of all bytes in the buffer.
 */
uint8_t swicc_ck(uint8_t const *const buf_raw, uint16_t const buf_raw_len);

/**
 * @brief Converts a string of hex nibbles (encoded as ASCII), into a byte
 * array.
 * @param hexstr
 * @param hexstr_len
 * @param bytearr Where to write the byte array.
 * @param bytearr_len Must hold the allocated size of the byte array buffer. On
 * success, will receive the number of bytes written to the byte array buffer.
 * @return Return code.
 */
swicc_ret_et swicc_hexstr_bytearr(char const *const hexstr,
                                  uint32_t const hexstr_len,
                                  uint8_t *const bytearr,
                                  uint32_t *const bytearr_len);

/**
 * @brief Perform a hard reset of the swICC state.
 * @param swicc_state
 * @return Return code.
 * @note No other state is kept internally so this is sufficient as an analog to
 * the deactivation (power off) of a real ICC.
 */
swicc_ret_et swicc_reset(swicc_st *const swicc_state);

/**
 * @brief Perform cleanup for a swICC that is being destroyed.
 * @param swicc_state
 * @note After this succeeds, operations involving the swICC state will become
 * undefined.
 */
void swicc_terminate(swicc_st *const swicc_state);

/**
 * @brief Gets the current state of the FSM.
 * @param swicc_state
 * @param state
 */
void swicc_fsm_state(swicc_st *const swicc_state,
                     swicc_fsm_state_et *const state);

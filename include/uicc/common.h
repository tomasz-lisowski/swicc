#pragma once

#include <stdbool.h>
#include <stdint.h>

#define UICC_DATA_MAX_SHRT 256U
#define UICC_DATA_MAX_LONG 65536U
#define UICC_DATA_MAX UICC_DATA_MAX_SHRT

/**
 * All possible return codes that can get returned from the functions of this
 * library.
 */
typedef enum uicc_ret_e
{
    UICC_RET_UNKNOWN = 0,
    UICC_RET_SUCCESS =
        1,              /* In principle =1, allows for use as 'if' condition. */
    UICC_RET_ERROR,     /* Unspecified error (non-critical). */
    UICC_RET_PARAM_BAD, /* Generic error to indicate the parameter was bad. */

    UICC_RET_APDU_HDR_TOO_SHORT,
    UICC_RET_APDU_UNHANDLED,

    UICC_RET_APDU_RES_INVALID,
    UICC_RET_TPDU_HDR_TOO_SHORT,
    UICC_RET_BUFFER_TOO_SHORT,

    UICC_RET_PPS_INVALID, /* E.g. the check byte is incorrect etc... */
    UICC_RET_PPS_FAILED,  /* Request is handled but params are not accepted */

    UICC_RET_ATR_INVALID,  /* E.g. the ATR might not contain madatory fields or
                              is malformed. */
    UICC_RET_FS_NOT_FOUND, /* Requested FS item is not present. */

    UICC_RET_DATO_END, /* Reached end of buffer/data. */
} uicc_ret_et;

/**
 * Typedef these to avoid including and creating circular deps.
 */
typedef struct uicc_s uicc_st;
typedef struct uicc_fs_file_s uicc_fs_file_st;
typedef enum uicc_fsm_state_e uicc_fsm_state_et;

/**
 * @brief Compute the elementary time unit (ETU) as described in ISO 7816-3:2006
 * p.13 sec.7.1.
 * @param etu Where the computed ETU will be written.
 * @param fi The clock rate conversion integer (Fi).
 * @param di The baud rate adjustment integer (Di).
 * @param fmax The maximum supported clock frequency (f(max)).
 */
void uicc_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
              uint32_t const fmax);

/**
 * @brief Compute check byte for a buffer. This means the result of XOR'ing all
 * bytes together. ISO 7816-3:2006 p.18 sec.8.2.5.
 * @param buf_raw Buffer.
 * @param buf_raw_len Length of the data in the buffer.
 * @return XOR of all bytes in the buffer.
 */
uint8_t uicc_ck(uint8_t const *const buf_raw, uint16_t const buf_raw_len);

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
uicc_ret_et uicc_hexstr_bytearr(char const *const hexstr,
                                uint32_t const hexstr_len,
                                uint8_t *const bytearr,
                                uint32_t *const bytearr_len);

/**
 * @brief Perform a hard reset of the UICC state.
 * @param uicc_state
 * @return Return code.
 * @note No other state is kept internally so this is sufficient as an analog to
 * the deactivation of a UICC.
 */
uicc_ret_et uicc_reset(uicc_st *const uicc_state);

/**
 * @brief Perform cleanup for a UICC that is being destroyed.
 * @param uicc_state
 * @note After this succeeds, operations involving the UICC state will become
 * undefined.
 */
void uicc_terminate(uicc_st *const uicc_state);

/**
 * @brief Gets the current state of the FSM.
 * @param uicc_state
 * @param state
 */
void uicc_fsm_state(uicc_st *const uicc_state, uicc_fsm_state_et *const state);

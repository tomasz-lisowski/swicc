#include <stdbool.h>
#include <stdint.h>

#pragma once

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
    UICC_RET_APDU_DATA_WAIT, /* There will be (more) data coming for the current
                                command. */
    UICC_RET_APDU_RES_INVALID,
    UICC_RET_TPDU_HDR_TOO_SHORT,
    UICC_RET_BUFFER_TOO_SHORT,

    UICC_RET_FSM_TRANSITION_WAIT, /* Wait for I/O state change then run FSM. */
    UICC_RET_FSM_TRANSITION_NOW,  /* Without waiting, let the FSM run again. */

    UICC_RET_PPS_INVALID, /* E.g. the check byte is incorrect etc... */
    UICC_RET_PPS_FAILED,  /* Request is handled but params are not accepted */

    UICC_RET_ATR_INVALID,  /* E.g. the ATR might not contain madatory fields or
                              is malformed. */
    UICC_RET_FS_NOT_FOUND, /* E.g. SELECT with FID was done but a file with
                            the given FID does not exist. */

    UICC_RET_DATO_END, /* Reached end of buffer/data. */
} uicc_ret_et;

/**
 * Since many modules will need this, it is typedef'd here to avoid circular
 * includes.
 */
typedef struct uicc_s uicc_st;

/**
 * Need it here to declaring it.
 * XXX: It's important this is updated or removed if it's no longer needed or if
 * the struct changed definition.
 */
typedef struct uicc_fs_file_hdr_s uicc_fs_file_hdr_st;

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
uint8_t uicc_tck(uint8_t const *const buf_raw, uint16_t const buf_raw_len);

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
 * @return Return code.
 * @note After this succeeds, operations involving the UICC state will become
 * undefined.
 */
uicc_ret_et uicc_terminate(uicc_st *const uicc_state);

/**
 * @brief Create an LCS byte for a file.
 * @param file
 * @param lcs
 * @return Return code.
 * @note Done according to ISO 7816-4:2020 p.31 sec.7.4.10 table.15.
 */
uicc_ret_et uicc_file_lcs(uicc_fs_file_hdr_st const *const file,
                          uint8_t *const lcs);

/**
 * @brief Create a file descriptor byte for a file.
 * @param file
 * @param file_descr
 * @return Return code.
 * @note Done according to ISO 7816-4:2020 p.29 sec.7.4.5 table.12.
 */
uicc_ret_et uicc_file_descr(uicc_fs_file_hdr_st const *const file,
                            uint8_t *const file_descr);

/**
 * @brief Create a data coding byte for a file.
 * @param file
 * @param data_coding
 * @return Return code.
 * @note Done according to second software function table described in ISO
 * 7816-4:2020 p.123 sec.12.2.2.9 table.126.
 */
uicc_ret_et uicc_file_data_coding(uicc_fs_file_hdr_st const *const file,
                                  uint8_t *const data_coding);

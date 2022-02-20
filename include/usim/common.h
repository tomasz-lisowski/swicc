#include <stdint.h>

#pragma once

#define USIM_DATA_MAX_SHRT 256U
#define USIM_DATA_MAX_LONG 65536U
#define USIM_DATA_MAX USIM_DATA_MAX_SHRT

/**
 * All possible return codes that can get returned from the functions of this
 * library.
 */
typedef enum usim_ret_e
{
    USIM_RET_UNKNOWN = 0,
    USIM_RET_SUCCESS = 1,
    USIM_RET_APDU_CMD_TOO_SHORT,
    USIM_RET_TPDU_CMD_TOO_SHORT,
    USIM_RET_BUFFER_TOO_SHORT,

    USIM_RET_FSM_TRANSITION_WAIT, /* Wait for I/O state change then run FSM. */
    USIM_RET_FSM_TRANSITION_NOW,  /* Without waiting, let the FSM run again. */

    USIM_RET_PPS_INVALID, /* E.g. the check byte is incorrect etc... */
    USIM_RET_PPS_FAILED,  /* Request is handled but params are not accepted */
} usim_ret_et;

/**
 * Since many modules will need this, it is typedef'd here to avoid circular
 * includes.
 */
typedef struct usim_s usim_st;

/**
 * @brief Compute the elementary time unit (ETU) as described in ISO 7816-3:2006
 * p.13 sec.7.1.
 * @param etu Where the computed ETU will be written.
 * @param fi The clock rate conversion integer (Fi).
 * @param di The baud rate adjustment integer (Di).
 * @param fmax The maximum supported clock frequency (f(max)).
 */
void usim_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
              uint32_t const fmax);

#include <stdint.h>
#include <usim/common.h>

#pragma once

/**
 * Number of unique configurations of the transmission protocol.
 *
 * ISO 7816-3:2006 p.18-19 sec.8.3
 */
#define USIM_TP_CONF_NUM 16U

/**
 * Index in the lookup tables for Fi, Di, f(max) to get the default values (used
 * right after a reset of any kind).
 *
 * ISO 7816-3:2006 p.18 sec.8.3
 */
#define USIM_TP_CONF_DEFAULT 1U

/**
 * An enum for representing the state of each contact state (apart from I/O and
 * ground).
 * @note There are 4 definitions per contact to get both a member for the
 * contact ID and the assigned purpose as well as the 'valid' flag to have the
 * ability to indicate if a contact could be read or should be written to.
 *
 * ISO 7816-3:2006:2006 p.6 sec.5.1.1
 */
typedef enum usim_io_cont_e
{
    USIM_IO_CONT_VCC = 1U << 1U,
    USIM_IO_CONT_C1 = 1U << 1U,
    USIM_IO_CONT_VALID_VCC = 1U << 2U,
    USIM_IO_CONT_VALID_C1 = 1U << 2U,

    USIM_IO_CONT_RST = 1U << 3U,
    USIM_IO_CONT_C2 = 1U << 3U,
    USIM_IO_CONT_VALID_RST = 1U << 4U,
    USIM_IO_CONT_VALID_C2 = 1U << 4U,

    USIM_IO_CONT_CLK = 1U << 5U,
    USIM_IO_CONT_C3 = 1U << 5U,
    USIM_IO_CONT_VALID_CLK = 1U << 6U,
    USIM_IO_CONT_VALID_C3 = 1U << 6U,

    USIM_IO_CONT_SPU = 1U << 7U,
    USIM_IO_CONT_C6 = 1U << 7U,
    USIM_IO_CONT_VALID_SPU = 1U << 8U,
    USIM_IO_CONT_VALID_C6 = 1U << 8U,

    USIM_IO_CONT_IO = 1U << 9U,
    USIM_IO_CONT_C7 = 1U << 9U,
    USIM_IO_CONT_VALID_IO = 1U << 10U,
    USIM_IO_CONT_VALID_C7 = 1U << 10U,

    /* C5 is GND */
    /* C4 is reserved */
    /* C8 is reserved */
} usim_io_contact_et;

#define USIM_IO_CONT_VALID_ALL                                                 \
    (USIM_IO_CONT_VALID_VCC | USIM_IO_CONT_VALID_RST |                         \
     USIM_IO_CONT_VALID_CLK | USIM_IO_CONT_VALID_SPU | USIM_IO_CONT_VALID_IO)

/**
 * Lookup tables for resolving an integer of a parameter to the parameter value
 * itself.
 */
extern uint16_t const usim_lookup_fi[USIM_TP_CONF_NUM];
extern uint8_t const usim_lookup_di[USIM_TP_CONF_NUM];
extern uint32_t const usim_lookup_fmax[USIM_TP_CONF_NUM];

/**
 * @brief Process (interface requested) changes in state of the electrical
 * circuit state of the SIM.
 * @param usim_state A representation of the final state of USIM at this moment
 * in time.
 * @return Return code.
 * @note This should be called if the state of any of the contacts changes or if
 * data is received in which case receive it all into a buffer in the I/O
 * struct and then call this function.
 */
usim_ret_et usim_io(usim_st *const usim_state);

/**
 * @brief Perform a hard reset of the USIM state.
 * @param usim_state The USIM state to be reset.
 * @return Return code.
 * @note No other state is kept internally so this is sufficient as an analog to
 * the deactivation of a USIM.
 */
usim_ret_et usim_reset(usim_st *const usim_state);

#include "swicc/common.h"

#pragma once

/**
 * Number of unique configurations of the transmission protocol.
 * ISO/IEC 7816-3:2006 clause.8.3
 */
#define SWICC_TP_CONF_NUM 16U

/**
 * Index in the lookup tables for Fi, Di, f(max) to get the default values (used
 * right after a reset of any kind).
 * ISO/IEC 7816-3:2006 clause.8.3
 */
#define SWICC_TP_CONF_DEFAULT 1U

/**
 * Represent the state of each contact state (apart from I/O and ground).
 * @note There are 4 definitions per contact to get both a member for the
 * contact ID and the assigned purpose as well as the 'valid' flag to have the
 * ability to indicate if a contact could be read or should be written to.
 * ISO/IEC 7816-3:2006 clause.5.1.1.
 */
typedef enum swicc_io_cont_e
{
    SWICC_IO_CONT_VCC = 1U << 1U,
    SWICC_IO_CONT_C1 = 1U << 1U,
    SWICC_IO_CONT_VALID_VCC = 1U << 2U,
    SWICC_IO_CONT_VALID_C1 = 1U << 2U,

    SWICC_IO_CONT_RST = 1U << 3U,
    SWICC_IO_CONT_C2 = 1U << 3U,
    SWICC_IO_CONT_VALID_RST = 1U << 4U,
    SWICC_IO_CONT_VALID_C2 = 1U << 4U,

    SWICC_IO_CONT_CLK = 1U << 5U,
    SWICC_IO_CONT_C3 = 1U << 5U,
    SWICC_IO_CONT_VALID_CLK = 1U << 6U,
    SWICC_IO_CONT_VALID_C3 = 1U << 6U,

    /* C4 is reserved */
    /* C5 is GND */

    /**
     * ETSI TS 102 221 V16.4.0 also specifies that C6 is used for VPP
     * (programming voltage).
     */
    SWICC_IO_CONT_SPU = 1U << 11U,
    SWICC_IO_CONT_C6 = 1U << 11U,
    SWICC_IO_CONT_VALID_SPU = 1U << 12U,
    SWICC_IO_CONT_VALID_C6 = 1U << 12U,

    SWICC_IO_CONT_IO = 1U << 13U,
    SWICC_IO_CONT_C7 = 1U << 13U,
    SWICC_IO_CONT_VALID_IO = 1U << 14U,
    SWICC_IO_CONT_VALID_C7 = 1U << 14U,

    /* C8 is reserved */
} swicc_io_contact_et;

#define SWICC_IO_CONT_VALID_ALL                                                \
    (SWICC_IO_CONT_VALID_VCC | SWICC_IO_CONT_VALID_RST |                       \
     SWICC_IO_CONT_VALID_CLK | SWICC_IO_CONT_VALID_SPU |                       \
     SWICC_IO_CONT_VALID_IO)

/**
 * Lookup tables for resolving an integer of a parameter to the parameter value
 * itself.
 */
extern uint16_t const swicc_io_fi[SWICC_TP_CONF_NUM];
extern uint8_t const swicc_io_di[SWICC_TP_CONF_NUM];
extern uint32_t const swicc_io_fmax[SWICC_TP_CONF_NUM];

/**
 * @brief Process changes in the state of electrical contacts of the ICC
 * interface.
 * @param swicc_state
 * @return Return code.
 * @note Shall be called when any of the contacts change state or if the
 * requested amount of data has been received.
 */
void swicc_io(swicc_st *const swicc_state);

#include "uicc/common.h"

#pragma once
/**
 * @todo Verify that the expected number of RX'd bytes is the actual number
 * provided by the user.
 */

/**
 * Number of unique configurations of the transmission protocol.
 * ISO 7816-3:2006 p.18-19 sec.8.3
 */
#define UICC_TP_CONF_NUM 16U

/**
 * Index in the lookup tables for Fi, Di, f(max) to get the default values (used
 * right after a reset of any kind).
 *
 * ISO 7816-3:2006 p.18 sec.8.3
 */
#define UICC_TP_CONF_DEFAULT 1U

/**
 * An enum for representing the state of each contact state (apart from I/O and
 * ground).
 * @note There are 4 definitions per contact to get both a member for the
 * contact ID and the assigned purpose as well as the 'valid' flag to have the
 * ability to indicate if a contact could be read or should be written to.
 *
 * ISO 7816-3:2006 p.6 sec.5.1.1
 */
typedef enum uicc_io_cont_e
{
    UICC_IO_CONT_VCC = 1U << 1U,
    UICC_IO_CONT_C1 = 1U << 1U,
    UICC_IO_CONT_VALID_VCC = 1U << 2U,
    UICC_IO_CONT_VALID_C1 = 1U << 2U,

    UICC_IO_CONT_RST = 1U << 3U,
    UICC_IO_CONT_C2 = 1U << 3U,
    UICC_IO_CONT_VALID_RST = 1U << 4U,
    UICC_IO_CONT_VALID_C2 = 1U << 4U,

    UICC_IO_CONT_CLK = 1U << 5U,
    UICC_IO_CONT_C3 = 1U << 5U,
    UICC_IO_CONT_VALID_CLK = 1U << 6U,
    UICC_IO_CONT_VALID_C3 = 1U << 6U,

    /* C4 is reserved */
    /* C5 is GND */

    /**
     * ETSI TS 102 221 V16.4.0 also specifies that C6 is used for VPP
     * (programming voltage).
     */
    UICC_IO_CONT_SPU = 1U << 11U,
    UICC_IO_CONT_C6 = 1U << 11U,
    UICC_IO_CONT_VALID_SPU = 1U << 12U,
    UICC_IO_CONT_VALID_C6 = 1U << 12U,

    UICC_IO_CONT_IO = 1U << 13U,
    UICC_IO_CONT_C7 = 1U << 13U,
    UICC_IO_CONT_VALID_IO = 1U << 14U,
    UICC_IO_CONT_VALID_C7 = 1U << 14U,

    /* C8 is reserved */
} uicc_io_contact_et;

#define UICC_IO_CONT_VALID_ALL                                                 \
    (UICC_IO_CONT_VALID_VCC | UICC_IO_CONT_VALID_RST |                         \
     UICC_IO_CONT_VALID_CLK | UICC_IO_CONT_VALID_SPU | UICC_IO_CONT_VALID_IO)

/**
 * Lookup tables for resolving an integer of a parameter to the parameter value
 * itself.
 */
extern uint16_t const uicc_io_fi[UICC_TP_CONF_NUM];
extern uint8_t const uicc_io_di[UICC_TP_CONF_NUM];
extern uint32_t const uicc_io_fmax[UICC_TP_CONF_NUM];

/**
 * @brief Process (interface requested) changes in state of the electrical
 * circuit state of the UICC.
 * @param uicc_state
 * @return Return code.
 * @note Shall be called when any of the contacts change state or if the
 * requested amount of data has been received.
 */
void uicc_io(uicc_st *const uicc_state);

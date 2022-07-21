#pragma once
/**
 * Mocking operations that would have otherwise be done through a combination of
 * contact state changes and FSM transitions to simplify the interface if there
 * is no access to electrical contacts e.g. when injecting data directly into
 * the firmware via another interface.
 */

#include "swicc/common.h"

/**
 * @brief Perform a cold reset of the swICC.
 * @param[in, out] swicc_state
 * @param mock_pps If this method should also perform the PPS negotiation
 * (=true) or not (=false).
 * @return Return code.
 */
swicc_ret_et swicc_mock_reset_cold(swicc_st *const swicc_state,
                                   bool const mock_pps);

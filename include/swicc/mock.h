#pragma once
/**
 * Mocking operations that would have otherwise be done through a combination of
 * contact state changes and FSM transitions to simplify the interface if there
 * is no access to electrical contacts.
 */

#include "swicc/common.h"

swicc_ret_et swicc_mock_reset_cold(swicc_st *const swicc_state,
                                   bool const mock_pps);

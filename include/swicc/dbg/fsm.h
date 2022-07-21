#pragma once

#include "swicc/common.h"
#include "swicc/fsm.h"

/**
 * @brief Get a string of the current FSM state.
 * @param[in] fsm_state
 * @return Return code.
 */
char const *swicc_dbg_fsm_state_str(swicc_fsm_state_et const fsm_state);

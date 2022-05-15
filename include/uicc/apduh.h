#pragma once
/**
 * APDUH implements handlers for all interindustry APDUs and acts as a demux for
 * running the correct handler for a given command (be it interindustry or
 * proprietary using a registered handler).
 */

#include "uicc/apdu.h"
#include "uicc/common.h"

/* APDU handler. */
typedef uicc_ret_et uicc_apduh_ft(uicc_st *const uicc_state,
                                  uicc_apdu_cmd_st const *const cmd,
                                  uicc_apdu_res_st *const res);

/**
 * @brief All APDUs in the proprietary class require non-interindusry
 * implementations for handlers. The handler passed to this function is the
 * function that will get these proprietary messages and is expected to handle
 * them.
 * @param uicc_state
 * @param handler Handler for all proprietary messages.
 * @return Return code.
 */
uicc_ret_et uicc_apduh_pro_register(uicc_st *const uicc_state,
                                    uicc_apduh_ft *const handler);

/**
 * @brief Handle all APDUs.
 * @param uicc_state
 * @param cmd Command to handle.
 * @param res Response to the command.
 * @return Return code.
 */
uicc_apduh_ft uicc_apduh_demux;
uicc_ret_et uicc_apduh_demux(uicc_st *const uicc_state,
                             uicc_apdu_cmd_st const *const cmd,
                             uicc_apdu_res_st *const res);

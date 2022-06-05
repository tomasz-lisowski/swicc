#pragma once
/**
 * APDUH implements handlers for all interindustry APDUs and acts as a demux for
 * running the correct handler for a given command (be it interindustry or
 * proprietary using a registered handler).
 */

#include "uicc/apdu.h"
#include "uicc/common.h"

/**
 * @brief APDU handler.
 * @param uicc_state
 * @param cmd Command to handle.
 * @param res Response to the command.
 * @param procedure_count Informs the handler about the number of procedure
 * bytes already sent.
 * @return Return code.
 */
typedef uicc_ret_et uicc_apduh_ft(uicc_st *const uicc_state,
                                  uicc_apdu_cmd_st const *const cmd,
                                  uicc_apdu_res_st *const res,
                                  uint32_t const procedure_count);

/**
 * @brief All APDUs in the proprietary class require non-interindustry
 * implementations for handlers. The handler passed to this function is the
 * function that will get these proprietary messages and is expected to handle
 * them.
 * @note An attempt to handle interindustry messages using the proprietary
 * handler will be done before running interindustry handlers in order to give a
 * chance to override the default implementastion.
 * @param uicc_state
 * @param handler Handler for all proprietary messages.
 * @return Return code.
 */
uicc_ret_et uicc_apduh_pro_register(uicc_st *const uicc_state,
                                    uicc_apduh_ft *const handler);

/**
 * @brief Handle all APDUs.
 * @param uicc_state
 * @param cmd
 * @param res
 * @param procedure_count
 * @return Return code.
 */
uicc_apduh_ft uicc_apduh_demux;
uicc_ret_et uicc_apduh_demux(uicc_st *const uicc_state,
                             uicc_apdu_cmd_st const *const cmd,
                             uicc_apdu_res_st *const res,
                             uint32_t const procedure_count);

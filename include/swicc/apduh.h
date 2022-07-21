#pragma once
/**
 * APDUH implements handlers for all interindustry APDUs and acts as a demux for
 * running the correct handler for a given command (be it interindustry or
 * proprietary using a registered handler).
 */

#include "swicc/apdu.h"
#include "swicc/common.h"

/**
 * APDU handlers often set SW1, SW2, and data length just before returning. This
 * acts as a shothand for that.
 */
#define SWICC_APDUH_RES(res, sw1_val, sw2_val, res_len)                        \
    {                                                                          \
        res->sw1 = sw1_val;                                                    \
        res->sw2 = sw2_val;                                                    \
        res->data.len = res_len;                                               \
    }                                                                          \
    while (0)

/**
 * @brief APDU handler.
 * @param[in, out] swicc_state
 * @param[in] cmd Command to handle.
 * @param[out] res Response to the command.
 * @param[in] procedure_count Informs the handler about the number of procedure
 * bytes already sent.
 * @return Return code.
 * @warning When responding with a procedure byte (which is not a status), the
 * data length of the response indicates the number of bytes that shall be
 * received before the handler is called again. Note that the actual number of
 * bytes read is not guaranteed and can be less or more than requested.
 */
typedef swicc_ret_et swicc_apduh_ft(swicc_st *const swicc_state,
                                    swicc_apdu_cmd_st const *const cmd,
                                    swicc_apdu_res_st *const res,
                                    uint32_t const procedure_count);

/**
 * @brief All APDUs in the proprietary class require non-interindustry
 * implementations for handlers. The handler passed to this function is the
 * function that will get these proprietary messages and is expected to handle
 * them.
 * @param[in, out] swicc_state
 * @param[in] handler Handler for all proprietary messages.
 * @return Return code.
 * @note An attempt to handle interindustry messages using the proprietary
 * handler will be done before running interindustry handlers in order to give a
 * chance to override the default implementastion.
 */
swicc_ret_et swicc_apduh_pro_register(swicc_st *const swicc_state,
                                      swicc_apduh_ft *const handler);

/**
 * @brief Handle all APDUs.
 */
swicc_apduh_ft swicc_apduh_demux;
swicc_ret_et swicc_apduh_demux(swicc_st *const swicc_state,
                               swicc_apdu_cmd_st const *const cmd,
                               swicc_apdu_res_st *const res,
                               uint32_t const procedure_count);

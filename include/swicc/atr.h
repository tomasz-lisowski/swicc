#pragma once
/**
 * ATR = Answer-to-reset
 */

#include "swicc/common.h"

#define SWICC_ATR_LEN 25

/**
 * Card ATR for use when the interface shall either enter a negotiation (PPS
 * exchange) or select the first offered transmission protofcol i.e. T=0.
 */
extern uint8_t const swicc_atr[SWICC_ATR_LEN];

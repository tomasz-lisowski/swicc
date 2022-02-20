#include <stdint.h>
#include <usim/common.h>

#pragma once
/**
 * ATR = Answer-to-reset
 */

#define USIM_ATR_LEN 10

/**
 * Card ATR for use when the interface shall either enter a negotiation (PPS
 * exchange) or select the first offered transmission protofcol i.e. T=0.
 */
extern uint8_t const usim_atr[USIM_ATR_LEN];

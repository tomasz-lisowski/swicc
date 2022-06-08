#pragma once
/**
 * ATR = Answer-to-reset
 */

#include "uicc/common.h"

#define UICC_ATR_LEN 25

/**
 * Card ATR for use when the interface shall either enter a negotiation (PPS
 * exchange) or select the first offered transmission protofcol i.e. T=0.
 */
extern uint8_t const uicc_atr[UICC_ATR_LEN];

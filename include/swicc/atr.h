#pragma once
/**
 * ATR = Answer-to-reset
 */

#include "swicc/common.h"

#define SWICC_ATR_LEN 25

/**
 * Card ATR is the first thing sent in the comms between the terminal and ICC.
 * It describes the supported transmission protocols and other ICC
 * configurations.
 */
extern uint8_t const swicc_atr[SWICC_ATR_LEN];

#pragma once

#include "swicc/common.h"

#define CLR_DEF "\x1B[0m"
#define CLR_RED "\x1B[31m"
#define CLR_GRN "\x1B[32m"
#define CLR_YEL "\x1B[33m"
#define CLR_BLU "\x1B[34m"
#define CLR_MAG "\x1B[35m"
#define CLR_CYN "\x1B[36m"
#define CLR_WHT "\x1B[37m"

#define CLR_TXT(clr, txt) clr txt CLR_DEF
#define CLR_VAL(txt) CLR_CYN txt CLR_DEF
#define CLR_KND(txt) CLR_GRN txt CLR_DEF

char const *swicc_dbg_ret_str(swicc_ret_et const ret);

#include "uicc/common.h"
#include "uicc/fs/disk.h"

#pragma once

uicc_ret_et uicc_dbg_disk_str(char *const buf_str, uint16_t *const buf_str_len,
                              uicc_disk_st const *const disk);

#include "uicc/common.h"
#include "uicc/fs.h"

#pragma once

uicc_ret_et uicc_dbg_fs_str(char *const buf_str, uint16_t *const buf_str_len,
                            uicc_fs_disk_st const *const disk);

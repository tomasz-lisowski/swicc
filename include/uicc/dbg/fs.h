#pragma once

#include "uicc/common.h"
#include "uicc/fs.h"

uicc_ret_et uicc_dbg_disk_str(char *const buf_str, uint16_t *const buf_str_len,
                              uicc_disk_st const *const disk);

char const *uicc_dbg_item_type_str(uicc_fs_item_type_et const item_type);

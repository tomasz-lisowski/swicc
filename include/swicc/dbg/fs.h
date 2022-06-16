#pragma once

#include "swicc/common.h"
#include "swicc/fs.h"

swicc_ret_et swicc_dbg_disk_str(char *const buf_str,
                                uint16_t *const buf_str_len,
                                swicc_disk_st const *const disk);

char const *swicc_dbg_item_type_str(swicc_fs_item_type_et const item_type);

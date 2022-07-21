#pragma once

#include "swicc/common.h"
#include "swicc/fs.h"

/**
 * @brief Convert disk to a string.
 * @param[out] buf_str Where to write the disk string.
 * @param[in, out] buf_str_len Must contain the maximum string length. Receives
 * actual disk string on success.
 * @param[in] disk The disk to convert to a string.
 * @return Return code.
 */
swicc_ret_et swicc_dbg_disk_str(char *const buf_str,
                                uint16_t *const buf_str_len,
                                swicc_disk_st const *const disk);

/**
 * @brief Get a string for an item type.
 * @param[in] item_type
 * @return Pointer to a constant type string, do not free, just forget it.
 */
char const *swicc_dbg_item_type_str(swicc_fs_item_type_et const item_type);

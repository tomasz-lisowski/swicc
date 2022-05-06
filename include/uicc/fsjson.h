#pragma once

#include "uicc/fs.h"

/**
 * @brief Creates a UICC FS (in-memory) from a JSON definition of the file
 * system.
 * @param disk Disk struct to populate.
 * @param disk_json_path Path to the JSON file describing the disk.
 * @return Return code.
 */
uicc_ret_et uicc_fsjson_disk_create(uicc_fs_disk_st *const disk,
                                    char const *const disk_json_path);

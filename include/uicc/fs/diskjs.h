#pragma once

#include "uicc/common.h"
#include "uicc/fs/disk.h"

/**
 * @brief Creates a UICC FS (in-memory) from a JSON definition of the file
 * system.
 * @param disk Disk struct to populate.
 * @param disk_json_path Path to the JSON file describing the disk.
 * @return Return code.
 */
uicc_ret_et uicc_diskjs_disk_create(uicc_disk_st *const disk,
                                    char const *const disk_json_path);

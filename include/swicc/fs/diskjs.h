#pragma once

#include "swicc/common.h"
#include "swicc/fs/disk.h"

/**
 * @brief Creates a swICC FS (in-memory) disk from a JSON definition of the
 * file system.
 * @param[out] disk Disk struct to populate.
 * @param[in] disk_json_path Path to the JSON file describing the disk.
 * @return Return code.
 */
swicc_ret_et swicc_diskjs_disk_create(swicc_disk_st *const disk,
                                      char const *const disk_json_path);

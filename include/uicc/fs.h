#pragma once

#include "uicc/fs/disk.h"
#include "uicc/fs/diskjs.h"
#include "uicc/fs/va.h"

/**
 * @brief Mount the given disk in the UICC.
 * @param uicc_state
 * @param disk
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_mount(uicc_st *const uicc_state,
                               uicc_disk_st *const disk);

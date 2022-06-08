#pragma once

#include "uicc/fs/disk.h"
#include "uicc/fs/diskjs.h"
#include "uicc/fs/va.h"

/* File descriptor. */
#define UICC_FS_FILE_DESCR_LEN_MAX 5U

/**
 * @brief Mount the given disk in the UICC.
 * @param uicc_state
 * @param disk
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_mount(uicc_st *const uicc_state,
                               uicc_disk_st *const disk);

/**
 * @brief Create an LCS byte for a file.
 * @param file
 * @param lcs
 * @return Return code.
 * @note Done according to ISO 7816-4:2020 p.31 sec.7.4.10 table.15.
 */
uicc_ret_et uicc_fs_file_lcs(uicc_fs_file_st const *const file,
                             uint8_t *const lcs);

/**
 * @brief Create a file descriptor for a given file.
 * @param tree Tree containing the file.
 * @param file
 * @param buf Where to write the file descriptor.
 * @param descr_len Length of the file descriptor written into the buffer will
 * be written here.
 * @return Return code.
 */
uicc_ret_et uicc_fs_file_descr(
    uicc_disk_tree_st const *const tree, uicc_fs_file_st const *const file,
    uint8_t buf[static const UICC_FS_FILE_DESCR_LEN_MAX],
    uint8_t *const descr_len);

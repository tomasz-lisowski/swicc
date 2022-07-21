#pragma once

#include "swicc/fs/disk.h"
#include "swicc/fs/diskjs.h"
#include "swicc/fs/va.h"

/* File descriptor. */
#define SWICC_FS_FILE_DESCR_LEN_MAX 5U

/**
 * @brief Mount the given disk in the swICC.
 * @param[in, out] swicc_state
 * @param[in] disk
 * @return Return code.
 */
swicc_ret_et swicc_fs_disk_mount(swicc_st *const swicc_state,
                                 swicc_disk_st *const disk);

/**
 * @brief Create an LCS byte for a file.
 * @param[in] file
 * @param[out] lcs
 * @return Return code.
 * @note Done according to ISO/IEC 7816-4:2020 p.31 sec.7.4.10 table.15.
 */
swicc_ret_et swicc_fs_file_lcs(swicc_fs_file_st const *const file,
                               uint8_t *const lcs);

/**
 * @brief Create a file descriptor for a given file.
 * @param[in] tree Tree containing the file.
 * @param[in] file
 * @param[out] buf Where to write the file descriptor.
 * @param[out] descr_len Length of the file descriptor written into the buffer
 * will be written here.
 * @return Return code.
 */
swicc_ret_et swicc_fs_file_descr(
    swicc_disk_tree_st const *const tree, swicc_fs_file_st const *const file,
    uint8_t buf[static const SWICC_FS_FILE_DESCR_LEN_MAX],
    uint8_t *const descr_len);

#pragma once

#include "uicc/common.h"
#include "uicc/fs/common.h"
#include "uicc/fs/disk.h"

/**
 * @todo Implement selection based on an occurrence enum i.e. first, last, and
 * next.
 * @todo Add ability to 'dry-select' into a copy of VA (transactional system) so
 * that instructions can create these temporary VAs and either submit or reject
 * their changes based on if they fail or not.
 */

/**
 * For a logical channel, a validity area (VA) summarizes the result
 * of all successful file selections. ISO 7816-4:2020 p.22 sec.7.2.1.
 */
typedef struct uicc_va_s
{
    uicc_disk_tree_st *cur_tree;
    uicc_fs_file_st cur_root;
    uicc_fs_file_st cur_df;
    uicc_fs_file_st cur_ef;
    /**
     * 'curFile' described in ISO 7816-4:2020 p.22 sec.7.2.1 is
     * skipped because it will be computed using 'cur_df' and
     * 'cur_ef'.
     */
    uicc_fs_rcrd_st cur_rcrd;

    /**
     * @todo Implement current data and DOs.
     */
    /* uicc_fs_data_st cur_data; */
    /* uicc_fs_do_st cur_do_constr; */
    /* uicc_fs_do_st cur_do_prim; */
    /**
     * 'curDO' described in ISO 7816-4:2020 p.22 sec.7.2.1 is
     * skipped because it will be computed using 'cur_do_constr' and
     * 'cur_do_prim'.
     */
} uicc_va_st;

/**
 * @brief Reset the VA to a state expected right after startup of a UICC i.e.
 * with the MF (3F00) selected.
 * @param fs
 * @return Return code.
 */
uicc_ret_et uicc_va_reset(uicc_fs_st *const fs);

/**
 * @brief Select an ADF (application) by its AID (application ID).
 * @param fs
 * @param aid Buffer containing the AID.
 * @param pix_len Length of the PIX component of the AID (the RID is always the
 * same length and the AID buffer must be guaranteed to contain at least the RID
 * plus as many PIX bytes as have been specified here).
 * @note The provided AID can be right-truncated in which case the selection
 * will be made based.
 */
uicc_ret_et uicc_va_select_adf(uicc_fs_st *const fs, uint8_t const *const aid,
                               uint32_t const pix_len);

/**
 * @brief Select a file by DF name.
 * @param fs
 * @param df_name String for the DF name. No need for it to be NULL-terminated.
 * @param df_name_len Length of the DF name string.
 * @return Return code.
 */
uicc_ret_et uicc_va_select_file_dfname(uicc_fs_st *const fs,
                                       char const *const df_name,
                                       uint32_t const df_name_len);

/**
 * @brief Select a file by file ID (file identifier).
 * @param fs
 * @param fid File identifier.
 * @return Return code.
 */
uicc_ret_et uicc_va_select_file_id(uicc_fs_st *const fs,
                                   uicc_fs_id_kt const fid);

/**
 * @brief Select a file by file SID (short file identifier).
 * @param fs
 * @param sid Short file identifier.
 * @return Return code.
 * @note Implicitly, the SID search is done on the current tree.
 */
uicc_ret_et uicc_va_select_file_sid(uicc_fs_st *const fs,
                                    uicc_fs_sid_kt const sid);

/**
 * @brief Select a file using a path.
 * @param fs
 * @param path Buffer which must be a concatenation of file IDs.
 * @param path_len Length of the path in bytes.
 * @return Return code.
 */
uicc_ret_et uicc_va_select_file_path(uicc_fs_st *const fs,
                                     uicc_fs_path_st const path);

/**
 * @brief Select a record by its record number i.e. index of the record in the
 * file.
 * @param fs
 * @param idx Record number/index.
 * @return Return code.
 */
uicc_ret_et uicc_va_select_record_idx(uicc_fs_st *const fs,
                                      uicc_fs_rcrd_idx_kt idx);

/**
 * @brief Select data with an index in a transparent buffer.
 * @param fs
 * @param offset_prel Parent-relative offset in the transparent buffer.
 * @return Return code.
 */
uicc_ret_et uicc_va_select_data_offset(uicc_fs_st *const fs,
                                       uint32_t offset_prel);

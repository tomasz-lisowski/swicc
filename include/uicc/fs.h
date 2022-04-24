#include "uicc/common.h"
#include <assert.h>

#pragma once

#define UICC_FS_MAGIC_LEN 8U
#define UICC_FS_MAGIC 0xAC55494343AC4653

typedef uint64_t uicc_fs_magic_kt;
static_assert(sizeof(uicc_fs_magic_kt) == UICC_FS_MAGIC_LEN,
              "UICC FS file magic data type length is not equal to the "
              "magic itself");

typedef uint16_t uicc_fs_fid_kt;
typedef uint16_t uicc_fs_record_id_kt;
typedef uint8_t uicc_fs_record_idx_kt;

typedef struct uicc_fs_disk_s
{
    uint8_t *buf;
    uint32_t len;
} uicc_fs_disk_st;

/* Describes a file in the file system. File can be an EF, DF, or ADF. */
typedef struct uicc_fs_file_s
{
    uint32_t offset;
    uint32_t size;
} uicc_fs_file_st;

/* Describes a record of an EF. */
typedef struct uicc_fs_record_s
{
    uicc_fs_record_id_kt id;
} uicc_fs_record_st;

/* Describes a transparent buffer A.k.a. a DataString. */
typedef struct uicc_fs_data_s
{
    uint32_t offset;
} uicc_fs_data_st;

/* Describes a data object or a part of one. */
typedef struct uicc_fs_do_s
{
    uint32_t parent_offset;
    uint32_t len;
} uicc_fs_do_st;

/**
 * For a logical channel, a validity area (VA) summarizes the result
 * of all successful file selections.
 * ISO 7816-4:2020 p.22 sec.7.2.1.
 */
typedef struct uicc_fs_va_s
{
    uicc_fs_file_st cur_adf;
    uicc_fs_file_st cur_df;
    uicc_fs_file_st cur_ef;
    /**
     * 'curFile' described in ISO 7816-4:2020 p.22 sec.7.2.1 is
     * skipped because it will be computed using 'cur_df' and
     * 'cur_ef'.
     */
    uicc_fs_record_st cur_record;
    uicc_fs_data_st cur_data;
    uicc_fs_do_st cur_do_constr;
    uicc_fs_do_st cur_do_prim;
    /**
     * 'curDO' described in ISO 7816-4:2020 p.22 sec.7.2.1 is
     * skipped because it will be computed using 'cur_do_constr' and
     * 'cur_do_prim'.
     */
} uicc_fs_va_st;

/**
 * @brief Reset the file system state to a state identical to the one at
 * startup but does not reload the file system.
 * @param uicc_state
 * @return Return code.
 */
uicc_ret_et uicc_fs_reset(uicc_st *const uicc_state);

/**
 * @brief Creates a UICC FS (in-memory) from a JSON definition of the file
 * system.
 * @param uicc_state
 * @param disk_json_path Path to the JSON file describing the disk.
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_create(uicc_st *const uicc_state,
                                char const *const disk_json_path);

/**
 * @brief Load a disk file (into memory) for use by this FS module.
 * @param uicc_state
 * @param disk_path Path to the disk file.
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_load(uicc_st *const uicc_state,
                              char const *const disk_path);
/**
 * @brief Save the in-memory disk to a specified file for persistence.
 * @param uicc_state
 * @param disk_path Path where to save the disk file.
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_save(uicc_st *const uicc_state,
                              char const *const disk_path);

/**
 * @brief Unload the in-memory disk and frees any memory used for storing the
 * FS.
 * @param uicc_state
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_unload(uicc_st *const uicc_state);

/**
 * @brief Select a file by DF name.
 * @param uicc_state
 * @param df_name String for the DF name. No need for it to be NULL-terminated.
 * @param df_name_len Length of the DF name string.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_file_dfname(uicc_st *const uicc_state,
                                       char const *const df_name,
                                       uint32_t const df_name_len);
/**
 * @brief Select a file by FID (file identifier).
 * @param uicc_state
 * @param fid File identifier.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_file_fid(uicc_st *const uicc_state,
                                    uicc_fs_fid_kt const fid);
/**
 * @brief Select a file by a path string.
 * @param uicc_state
 * @param path Buffer which must be a concatenation of FIDs.
 * @param path_len Length of the path in bytes.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_file_path(uicc_st *const uicc_state,
                                     char const *const path,
                                     uint32_t const path_len);
/**
 * @brief Select a DO by tag. This works because a given tag occurs exactly once
 * in a file.
 * @param uicc_state
 * @param tag The tag of the DO to select.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_do_tag(uicc_st *const uicc_state,
                                  uint16_t const tag);
/**
 * @brief Select a record by its unique identifier.
 * @param uicc_state
 * @param id Record ID.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_record_id(uicc_st *const uicc_state,
                                     uicc_fs_record_id_kt id);
/**
 * @brief Select a record by its record number i.e. index of the record in the
 * file.
 * @param uicc_state
 * @param idx Record number/index.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_record_idx(uicc_st *const uicc_state,
                                      uicc_fs_record_idx_kt idx);
/**
 * @brief Select data with an index in a transparent buffer.
 * @param uicc_state
 * @param offset Offset in the transparent buffer.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_data_offset(uicc_st *const uicc_state,
                                       uint32_t offset);

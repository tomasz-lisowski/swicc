#include "uicc/common.h"
#include <assert.h>

#pragma once

#define UICC_FS_MAGIC_LEN 8U
#define UICC_FS_MAGIC                                                          \
    {                                                                          \
        0xAC, 0x55, 0x49, 0x43, 0x43, 0xAC, 0x46, 0x53                         \
    }

#define UICC_FS_NAME_LEN_MAX 16U

typedef enum uicc_fs_item_type_e
{
    UICC_FS_ITEM_TYPE_INVALID,

    UICC_FS_ITEM_TYPE_FILE_MF,
    UICC_FS_ITEM_TYPE_FILE_ADF,
    UICC_FS_ITEM_TYPE_FILE_DF,
    UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT,
    UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED,
    // UICC_FS_ITEM_TYPE_FILE_EF_LINEARVARIABLE,
    UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC,
    // UICC_FS_ITEM_TYPE_FILE_EF_DATO,

    UICC_FS_ITEM_TYPE_DATO_BERTLV,
    UICC_FS_ITEM_TYPE_HEX,
    UICC_FS_ITEM_TYPE_ASCII,
} uicc_fs_item_type_et;

/**
 * Life cycle status as specified in ISO 7816-4:2020 p.31 sec.7.4.10 table.15.
 */
typedef enum uicc_fs_lcs_e
{
    UICC_FS_LCS_NINFO,        /* No info given */
    UICC_FS_LCS_CREAT,        /* Creation */
    UICC_FS_LCS_INIT,         /* Initialization */
    UICC_FS_LCS_OPER_ACTIV,   /* Operational + Activated */
    UICC_FS_LCS_OPER_DEACTIV, /* Operational + Deactivated */
    UICC_FS_LCS_TERM,         /* Termination */
} uicc_fs_lcs_et;

typedef uint16_t uicc_fs_id_kt;      /* ID like FID. */
typedef uint8_t uicc_fs_sid_kt;      /* Short ID like SFI. */
typedef uint16_t uicc_fs_rcrd_id_kt; /* Record 'number' or ID. */
typedef uint8_t uicc_fs_rcrd_idx_kt; /* Record index. */

/* The in-memory struct storing a UICC FS disk. */
typedef struct uicc_fs_disk_s
{
    uint8_t lutsid_count; /* SIDs are unique in a tree so can have >1 LUT. */
    uint8_t *buf;  /* This buffer holds the whole disk (including LUTs). */
    uint32_t size; /* The allocated size of the buffer. */

    /* Forest of trees containing files. */
    struct
    {
        /* Implicitly root begins at byte 0 of the index. */
        uint32_t len; /* Number of bytes in the buf that are occupied. */
    } root;

    /**
     * Lookup table for IDs.
     * XXX: The pointers must point to some portion of the buffer.
     */
    struct
    {
        uint32_t count;
        uicc_fs_id_kt *id;
        uint32_t *offset;
    } lutid;

    /**
     * Lookup table for SIDs.
     * XXX: The pointers must point to some portion of the buffer.
     */
    struct
    {
        uint32_t count;
        uicc_fs_sid_kt *sid;
        uint32_t *offset;
    } lutsid[];
} uicc_fs_disk_st;

/**
 * A represenatation of a header of any item in the UICC FS.
 * NOTE: This is the parsed representation.
 */
typedef struct uicc_fs_item_hdr_s
{
    uicc_fs_item_type_et type;
    uint32_t offset_start;
    uint32_t size;
    uicc_fs_lcs_et lcs;
} uicc_fs_item_hdr_st;
typedef struct uicc_fs_item_hdr_raw_s
{
    uint32_t size;
    uint8_t lcs;
    uint8_t type;
} __attribute__((packed)) uicc_fs_item_hdr_raw_st;

/**
 * Common header for all files (MF, EF, ADF, DF).
 * NOTE: This is the parsed representation.
 */
typedef struct uicc_fs_file_hdr_s
{
    uicc_fs_item_hdr_st item;
    uicc_fs_id_kt id;
    uicc_fs_sid_kt sid;
    char name[UICC_FS_NAME_LEN_MAX + 1U]; /* +1U for null-terminator */
} uicc_fs_file_hdr_st;
typedef struct uicc_fs_file_hdr_raw_s
{
    uicc_fs_item_hdr_raw_st item;
    uicc_fs_id_kt id;
    uicc_fs_sid_kt sid;
    char name[UICC_FS_NAME_LEN_MAX + 1U]; /* +1U for null-terminator */
} __attribute__((packed)) uicc_fs_file_hdr_raw_st;

/**
 * Header of a linear fixed EF.
 * NOTE: This is the parsed representation.
 */
typedef struct uicc_fs_ef_linearfixed_hdr_s
{
    uicc_fs_file_hdr_st file;
    uint8_t rcrd_size;
} uicc_fs_ef_linearfixed_hdr_st;
typedef struct uicc_fs_ef_linearfixed_hdr_raw_s
{
    uicc_fs_file_hdr_raw_st file;
    uint8_t rcrd_size;
} __attribute__((packed)) uicc_fs_ef_linearfixed_hdr_raw_st;
/**
 * Header of a cyclic EF is the same as for a linear fixed EF.
 */
typedef uicc_fs_ef_linearfixed_hdr_st uicc_fs_ef_cyclic_hdr_st;
typedef uicc_fs_ef_linearfixed_hdr_raw_st uicc_fs_ef_cyclic_hdr_raw_st;

/* Describes a record of an EF. */
typedef struct uicc_fs_rcrd_s
{
    uint32_t parent_offset_start;
    uicc_fs_rcrd_id_kt id;
    uicc_fs_rcrd_idx_kt idx;
    uint32_t offset_start;
    uint32_t size;
} uicc_fs_rcrd_st;

/* Describes a transparent buffer. */
typedef struct uicc_fs_data_s
{
    uint32_t parent_offset;
    uint32_t offset_start;
    uint32_t offset_select;
    uint32_t size;
} uicc_fs_data_st;

/* Describes a data object or a part of one. */
typedef struct uicc_fs_do_s
{
    uint32_t parent_offset;
    uint32_t offset_start;
    uint32_t size;
} uicc_fs_do_st;

/**
 * For a logical channel, a validity area (VA) summarizes the result
 * of all successful file selections.
 * ISO 7816-4:2020 p.22 sec.7.2.1.
 */
typedef struct uicc_fs_va_s
{
    uicc_fs_item_hdr_st cur_adf;
    uicc_fs_item_hdr_st cur_df;
    uicc_fs_item_hdr_st cur_ef;
    /**
     * 'curFile' described in ISO 7816-4:2020 p.22 sec.7.2.1 is
     * skipped because it will be computed using 'cur_df' and
     * 'cur_ef'.
     */
    uicc_fs_rcrd_st cur_rcrd;
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
                                    uicc_fs_id_kt const fid);
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
                                     uicc_fs_rcrd_id_kt id);
/**
 * @brief Select a record by its record number i.e. index of the record in the
 * file.
 * @param uicc_state
 * @param idx Record number/index.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_record_idx(uicc_st *const uicc_state,
                                      uicc_fs_rcrd_idx_kt idx);
/**
 * @brief Select data with an index in a transparent buffer.
 * @param uicc_state
 * @param offset Offset in the transparent buffer.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_data_offset(uicc_st *const uicc_state,
                                       uint32_t offset);

/**
 * @brief Parse an item header.
 * @param item_hdr_raw Item header to parse.
 * @param item_hdr Where to store the parsed item header.
 * @return Return code.
 * @note The 'offset' field of the parsed item will not be populated.
 */
uicc_ret_et uicc_fs_item_hdr_prs(
    uicc_fs_item_hdr_raw_st const *const item_hdr_raw,
    uicc_fs_item_hdr_st *const item_hdr);

/**
 * @brief Parse a file header.
 * @param file_hdr_raw File header to parse.
 * @param file_hdr Where to store the parsed file header.
 * @return Return code.
 * @note The item portion of the header will not be parsed.
 */
uicc_ret_et uicc_fs_file_hdr_prs(
    uicc_fs_file_hdr_raw_st const *const file_hdr_raw,
    uicc_fs_file_hdr_st *const file_hdr);

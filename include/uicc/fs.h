#include "uicc/common.h"
#include <assert.h>

#pragma once

#define UICC_FS_MAGIC_LEN 8U
#define UICC_FS_MAGIC                                                          \
    {                                                                          \
        0xAC, 0x55, 0x49, 0x43, 0x43, 0xAC, 0x46, 0x53                         \
    }
static_assert(sizeof((uint8_t[])UICC_FS_MAGIC) == UICC_FS_MAGIC_LEN,
              "Magic length macro not equal to the magic array length");

#define UICC_FS_NAME_LEN_MAX 16U
#define UICC_FS_DEPTH_MAX 3U

#define UICC_FS_ID_MISSING 0U
#define UICC_FS_SID_MISSING 0U

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
    // UICC_FS_LCS_NINFO,     /* No info given */
    // UICC_FS_LCS_CREAT,     /* Creation */
    // UICC_FS_LCS_INIT,      /* Initialization */
    UICC_FS_LCS_OPER_ACTIV,   /* Operational + Activated */
    UICC_FS_LCS_OPER_DEACTIV, /* Operational + Deactivated */
    UICC_FS_LCS_TERM,         /* Termination */
} uicc_fs_lcs_et;

typedef enum uicc_fs_path_type_e
{
    UICC_FS_PATH_TYPE_MF, /* Relative to the MF. */
    UICC_FS_PATH_TYPE_DF, /* Relative to the current DF. */
} uicc_fs_path_type_et;

typedef uint16_t uicc_fs_id_kt;      /* ID like FID. */
typedef uint8_t uicc_fs_sid_kt;      /* Short ID like SFI. */
typedef uint16_t uicc_fs_rcrd_id_kt; /* Record 'number' or ID. */
typedef uint8_t uicc_fs_rcrd_idx_kt; /* Record index. */

/* Representation of a LUT. */
typedef struct uicc_fs_lut_s
{
    uint8_t *buf1;
    uint8_t *buf2;

    uint32_t size_item1; /* Size of item in buffer 1. */
    uint32_t size_item2; /* Size of item in buffer 2. */

    /* Both buffers will be resized to contain the same amount of elements. */
    uint32_t count_max; /* Allocated size can hold this many items. */
    uint32_t count;     /* Number of items in the buffer. */
} uicc_fs_lut_st;

/* Representation of a tree in the root (forest). */
typedef struct uicc_fs_tree_s uicc_fs_tree_st;
struct uicc_fs_tree_s
{
    /**
     * The root-level trees like MF and ADFs are attached to a single
     * linked-list which forms the forest.
     */
    uicc_fs_tree_st *next;
    uint8_t *buf;  /* This buffer holds the whole disk (including LUTs). */
    uint32_t size; /* Allocated size. */
    uint32_t len;  /* Occupied size. */
    uicc_fs_lut_st lutsid;
};

/* The in-memory struct storing a UICC FS disk. */
typedef struct uicc_fs_disk_s
{
    uicc_fs_tree_st *root;
    uicc_fs_lut_st lutid; /* There is exactly one LUT for all IDs. */
} uicc_fs_disk_st;

/**
 * A represenatation of a header of any item in the UICC FS.
 */
typedef struct uicc_fs_item_hdr_s
{
    uint32_t size;
    uicc_fs_lcs_et lcs;
    uicc_fs_item_type_et type;

    /* Offset from top of the tree to the header of this item. */
    uint32_t offset_trel;

    /**
     * Offset from the start of the header of the parent to this item. A 0 means
     * the item has no parent.
     */
    uint32_t offset_prel;
} uicc_fs_item_hdr_st;
typedef struct uicc_fs_item_hdr_raw_s
{
    uint32_t size;
    uint8_t lcs;
    uint8_t type;
    uint32_t offset_prel;
} __attribute__((packed)) uicc_fs_item_hdr_raw_st;

/**
 * Common header for all files (MF, EF, ADF, DF).
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
    uint32_t size;
    uint32_t parent_offset_trel;
    uint32_t offset_prel_start;
    uicc_fs_rcrd_id_kt id;
    uicc_fs_rcrd_idx_kt idx;
} uicc_fs_rcrd_st;

/* Describes a transparent buffer. */
typedef struct uicc_fs_data_s
{
    uint32_t size;
    uint32_t parent_offset_trel;
    uint32_t offset_prel_start;
    uint32_t offset_prel_select;
} uicc_fs_data_st;

/* Describes a data object or a part of one. */
typedef struct uicc_fs_do_s
{
    uint32_t size;
    uint32_t parent_offset_trel;
    uint32_t offset_prel_start;
} uicc_fs_do_st;

typedef struct uicc_fs_path_s
{
    uicc_fs_path_type_et type;
    uint8_t *b;
    uint32_t len;
} uicc_fs_path_st;

/**
 * For a logical channel, a validity area (VA) summarizes the result
 * of all successful file selections. ISO 7816-4:2020 p.22 sec.7.2.1.
 */
typedef struct uicc_fs_va_s
{
    uicc_fs_tree_st *cur_adf;
    uicc_fs_file_hdr_st cur_df;
    uicc_fs_file_hdr_st cur_ef;
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
 * @brief Load a disk file (into memory) for use by this FS module.
 * @param uicc_state
 * @param disk_path Path to the disk file.
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_load(uicc_st *const uicc_state,
                              char const *const disk_path);

/**
 * @brief Unload the in-memory disk and frees any memory used for storing the
 * FS. Also ensure the validity area is reset properly.
 * @param uicc_state
 */
void uicc_fs_disk_unload(uicc_st *const uicc_state);

/**
 * @brief Free a disk struct.
 * @param disk
 */
void uicc_fs_disk_free(uicc_fs_disk_st *const disk);

/**
 * @brief Dealloc all disk buffers that hold forest data.
 * @param disk Disk for which to empty the forest/root.
 */
void uicc_fs_disk_root_empty(uicc_fs_disk_st *const disk);

/**
 * @brief Remove the SID LUT from a given tree.
 * @param tree The tree in which to empty the SID LUT.
 */
void uicc_fs_disk_lutsid_empty(uicc_fs_tree_st *const tree);

/**
 * @brief Dealloc all disk buffers that hold ID LUT data.
 * @param disk Disk for which to empty the ID LUT.
 */
void uicc_fs_disk_lutid_empty(uicc_fs_disk_st *const disk);

/**
 * @brief Save the disk as a UICC FS file to a specified file.
 * @param disk
 * @param disk_path Path where to save the disk file.
 * @return Return code.
 */
uicc_ret_et uicc_fs_disk_save(uicc_fs_disk_st *const disk,
                              char const *const disk_path);

/**
 * @brief Parse an item header.
 * @param item_hdr_raw Item header to parse.
 * @param item_hdr Where to store the parsed item header.
 * @return Return code.
 * @note The tree offset field of the parsed item will not be populated. The
 * offset to the parent will be parsed.
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

/**
 * @brief Create the LUT for IDs on the disk.
 * @param disk
 * @return Return code.
 */
uicc_ret_et uicc_fs_lutid_rebuild(uicc_fs_disk_st *const disk);

/**
 * @brief Create a LUT for SIDs for a tree.
 * @param disk
 * @param tree The tree for which to recreate the SID LUT.
 * @return Return code.
 */
uicc_ret_et uicc_fs_lutsid_rebuild(uicc_fs_disk_st *const disk,
                                   uicc_fs_tree_st *const tree);

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
 * @brief Select a file by file ID (file identifier).
 * @param uicc_state
 * @param fid File identifier.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_file_id(uicc_st *const uicc_state,
                                   uicc_fs_id_kt const fid);

/**
 * @brief Select a file using a path.
 * @param uicc_state
 * @param path Buffer which must be a concatenation of file IDs.
 * @param path_len Length of the path in bytes.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_file_path(uicc_st *const uicc_state,
                                     uicc_fs_path_st const path);

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
 * @param offset_prel Parent-relative offset in the transparent buffer.
 * @return Return code.
 */
uicc_ret_et uicc_fs_select_data_offset(uicc_st *const uicc_state,
                                       uint32_t offset_prel);

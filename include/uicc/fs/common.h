#pragma once

#include "uicc/common.h"
#include <assert.h>

/* It is typedef'd here to avoid circular includes. */
typedef struct uicc_fs_s uicc_fs_st;
typedef struct uicc_disk_tree_s uicc_disk_tree_st;

/**
 * Names and AIDs are equivalent in the standard. In this implementation, DFs
 * and MFs have a name which doesn't have any restrictions apart from length.
 * ADFs only have an AID which is more constrained to conform to the AID format.
 */
#define UICC_FS_NAME_LEN 16U
#define UICC_FS_DEPTH_MAX 3U
#define UICC_FS_ADF_AID_RID_LEN 5U
#define UICC_FS_ADF_AID_PIX_LEN 11U
#define UICC_FS_ADF_AID_LEN (UICC_FS_ADF_AID_RID_LEN + UICC_FS_ADF_AID_PIX_LEN)

/**
 * These values are used when a file does not have an ID and/or SID. This also
 * means that a valid ID or SID will not have these values.
 */
#define UICC_FS_ID_MISSING 0U
#define UICC_FS_SID_MISSING 0U

typedef enum uicc_fs_item_type_e
{
    UICC_FS_ITEM_TYPE_INVALID = 0U,

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
static_assert(UICC_FS_ITEM_TYPE_INVALID == 0U,
              "Invalid file type must be equal to 0");

#define UICC_FS_FILE_FOLDER_CHECK(file_p)                                      \
    (file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_MF ||                     \
     file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_DF ||                     \
     file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_ADF)

#define UICC_FS_FILE_EF_CHECK(file_p)                                          \
    (file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT ||         \
     file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||         \
     file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)

#define UICC_FS_FILE_EF_BERTLV_CHECK(file_p) (false)

#define UICC_FS_FILE_EF_BERTLV_NOT_CHECK(file_p)                               \
    (file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT ||         \
     file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||         \
     file_p->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)

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

/**
 * Occurrence for specifying how a selection should be done if multiple items
 * match.
 */
typedef enum uicc_fs_occ_e
{
    UICC_FS_OCC_FIRST,
    UICC_FS_OCC_LAST,
    UICC_FS_OCC_NEXT,
    UICC_FS_OCC_PREV,
} uicc_fs_occ_et;

typedef uint16_t uicc_fs_id_kt; /* ID like FID. */
typedef uint8_t uicc_fs_sid_kt; /* Short ID like SFI. */
typedef uint8_t
    uicc_fs_rcrd_idx_kt; /* Record index (NOT the record number whose indexing
                            begins at 1, the IDX begins at 0). */

/* A header of any item in the UICC FS. */
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

/* Common header for all files (MF, EF, ADF, DF). */
typedef struct uicc_fs_file_hdr_s
{
    uicc_fs_id_kt id;
    uicc_fs_sid_kt sid;
} uicc_fs_file_hdr_st;
typedef struct uicc_fs_file_hdr_raw_s
{
    uicc_fs_id_kt id;
    uicc_fs_sid_kt sid;
} __attribute__((packed)) uicc_fs_file_hdr_raw_st;

/* Extra header data of a MF. */
typedef struct uicc_fs_mf_hdr_s
{
    uint8_t name[UICC_FS_NAME_LEN];
} uicc_fs_mf_hdr_st;
typedef struct uicc_fs_mf_hdr_raw_s
{
    uint8_t name[UICC_FS_NAME_LEN];
} __attribute__((packed)) uicc_fs_mf_hdr_raw_st;

/* Extra header data of a DF. */
typedef struct uicc_fs_df_hdr_s
{
    uint8_t name[UICC_FS_NAME_LEN];
} uicc_fs_df_hdr_st;
typedef struct uicc_fs_df_hdr_raw_s
{
    uint8_t name[UICC_FS_NAME_LEN];
} __attribute__((packed)) uicc_fs_df_hdr_raw_st;

/* Extra header data of a ADF. */
typedef struct uicc_fs_adf_hdr_s
{
    /**
     * This is for the Application IDentifier which is present ONLY for ADFs.
     * ETSI TS 101 220 v15.2.0.
     */
    struct
    {
        /* Registered application provider IDentifier. */
        uint8_t rid[UICC_FS_ADF_AID_RID_LEN];

        /* Proprietary application Identifier eXtension. */
        uint8_t pix[UICC_FS_ADF_AID_PIX_LEN];
    } aid;
} uicc_fs_adf_hdr_st;
typedef struct uicc_fs_adf_hdr_raw_s
{
    struct
    {
        uint8_t rid[UICC_FS_ADF_AID_RID_LEN];
        uint8_t pix[UICC_FS_ADF_AID_PIX_LEN];
    } __attribute__((packed)) aid;
} __attribute__((packed)) uicc_fs_adf_hdr_raw_st;
static_assert(sizeof((uicc_fs_adf_hdr_raw_st){0U}.aid) == UICC_FS_ADF_AID_LEN,
              "AID has an unexpected size in the raw header struct of ADF");

/* Extra header data of a transparent EF. */
typedef struct uicc_fs_ef_transparent_hdr_s
{
} uicc_fs_ef_transparent_hdr_st;
typedef struct uicc_fs_ef_transparent_hdr_raw_s
{
} __attribute__((packed)) uicc_fs_ef_transparent_hdr_raw_st;

/* Extra header data of a linear-fixed EF. */
typedef struct uicc_fs_ef_linearfixed_hdr_s
{
    uint8_t rcrd_size;
} uicc_fs_ef_linearfixed_hdr_st;
typedef struct uicc_fs_ef_linearfixed_hdr_raw_s
{
    uint8_t rcrd_size;
} __attribute__((packed)) uicc_fs_ef_linearfixed_hdr_raw_st;

/* Extra header data of a cyclic EF. */
typedef struct uicc_fs_ef_cyclic_hdr_s
{
    uint8_t rcrd_size;
} uicc_fs_ef_cyclic_hdr_st;
typedef struct uicc_fs_ef_cyclic_hdr_raw_s
{
    uint8_t rcrd_size;
} __attribute__((packed)) uicc_fs_ef_cyclic_hdr_raw_st;

/* Describes a record of an EF. */
typedef struct uicc_fs_rcrd_s
{
    uicc_fs_rcrd_idx_kt idx;
} uicc_fs_rcrd_st;

typedef struct uicc_fs_path_s
{
    uicc_fs_path_type_et type;
    uint8_t *b;
    uint32_t len;
} uicc_fs_path_st;

/**
 * Outside of the disk and diskjs modules, this shall be the struct that
 * abstract away the implementation of files on disk.
 */
typedef struct uicc_fs_file_s
{
    uicc_fs_item_hdr_st hdr_item;
    uicc_fs_file_hdr_st hdr_file;
    union {
        uicc_fs_mf_hdr_st mf;
        uicc_fs_df_hdr_st df;
        uicc_fs_adf_hdr_st adf;
        uicc_fs_ef_transparent_hdr_st ef_transparent;
        uicc_fs_ef_linearfixed_hdr_st ef_linearfixed;
        uicc_fs_ef_cyclic_hdr_st ef_cyclic;
    } hdr_spec;
    uint32_t data_size;
    uint8_t *data;

    /* No module other than the FS should access this. */
    struct
    {
        uint8_t *hdr_raw;
    } internal;
} uicc_fs_file_st;
typedef struct uicc_fs_file_raw_s
{
    uicc_fs_item_hdr_raw_st hdr_item;
    uicc_fs_file_hdr_raw_st hdr_file;
    uint8_t data[];
} __attribute__((packed)) uicc_fs_file_raw_st;

extern uint32_t const uicc_fs_item_hdr_raw_size[];

/**
 * @brief Convert a raw item header to big endian.
 * @param item_hdr_raw
 */
void uicc_fs_item_hdr_raw_be(uicc_fs_item_hdr_raw_st *const item_hdr_raw);

/**
 * @brief Convert a raw file header to big endian.
 * @param file_hdr_raw
 */
void uicc_fs_file_hdr_raw_be(uicc_fs_file_hdr_raw_st *const file_hdr_raw);

/**
 * @brief Convert a raw MF header to big endian.
 * @param mf_hdr_raw
 */
void uicc_fs_mf_hdr_raw_be(uicc_fs_mf_hdr_raw_st *const mf_hdr_raw);

/**
 * @brief Convert a raw DF header to big endian.
 * @param df_hdr_raw
 */
void uicc_fs_df_hdr_raw_be(uicc_fs_df_hdr_raw_st *const df_hdr_raw);

/**
 * @brief Convert a raw transparent EF header to big endian.
 * @param ef_transparent_hdr_raw
 */
void uicc_fs_ef_transparent_hdr_raw_be(
    uicc_fs_ef_transparent_hdr_raw_st *const ef_transparent_hdr_raw);

/**
 * @brief Convert a raw linear-fixed EF header to big endian.
 * @param ef_linearfixed_hdr_raw
 */
void uicc_fs_ef_linearfixed_hdr_raw_be(
    uicc_fs_ef_linearfixed_hdr_raw_st *const ef_linearfixed_hdr_raw);

/**
 * @brief Convert a raw cyclic EF header to big endian.
 * @param ef_cyclic_hdr_raw
 */
void uicc_fs_ef_cyclic_hdr_raw_be(
    uicc_fs_ef_cyclic_hdr_raw_st *const ef_cyclic_hdr_raw);

/**
 * @brief Convert a raw ADF header to big endian.
 * @param adf_hdr_raw
 */
void uicc_fs_adf_hdr_raw_be(uicc_fs_adf_hdr_raw_st *const adf_hdr_raw);

/**
 * @brief Parse an item header.
 * @param item_hdr_raw Pointer to the raw item header.
 * @param offset_trel Tree-relative offset of the file.
 * @param item_hdr Where to store the parsed item header.
 */
void uicc_fs_item_hdr_prs(uicc_fs_item_hdr_raw_st const *const item_hdr_raw,
                          uint32_t const offset_trel,
                          uicc_fs_item_hdr_st *const item_hdr);

/**
 * @brief Parse a file header.
 * @param file_hdr_raw Pointer to the raw file header.
 * @param file_hdr Where to store the parsed file header.
 */
void uicc_fs_file_hdr_prs(uicc_fs_file_hdr_raw_st const *const file_hdr_raw,
                          uicc_fs_file_hdr_st *const file_hdr);

/**
 * @brief Parse some location of the tree as a file.
 * @param tree The tree containing the file.
 * @param offset_trel Offset (inside the tree buffer) to the beginning of the
 * file.
 * @param file Where the parsed file will be written.
 * @return Return code.
 */
uicc_ret_et uicc_fs_file_prs(uicc_disk_tree_st const *const tree,
                             uint32_t const offset_trel,
                             uicc_fs_file_st *const file);

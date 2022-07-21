#pragma once

#include "swicc/common.h"
#include <assert.h>

/* It is typedef'd here to avoid circular includes. */
typedef struct swicc_fs_s swicc_fs_st;
typedef struct swicc_disk_tree_s swicc_disk_tree_st;

/**
 * Names and AIDs are equivalent in the standard. In this implementation, DFs
 * and MFs have a name which doesn't have any restrictions apart from length.
 * ADFs only have an AID which is more constrained to conform to the AID format.
 */
#define SWICC_FS_NAME_LEN 16U
#define SWICC_FS_DEPTH_MAX 3U
#define SWICC_FS_ADF_AID_RID_LEN 5U
#define SWICC_FS_ADF_AID_PIX_LEN 11U
#define SWICC_FS_ADF_AID_LEN                                                   \
    (SWICC_FS_ADF_AID_RID_LEN + SWICC_FS_ADF_AID_PIX_LEN)

/**
 * These values are used when a file does not have an ID and/or SID. This also
 * means that a valid ID or SID will not have these values.
 */
#define SWICC_FS_ID_MISSING 0U
#define SWICC_FS_SID_MISSING 0U

typedef enum swicc_fs_item_type_e
{
    SWICC_FS_ITEM_TYPE_INVALID = 0U,

    SWICC_FS_ITEM_TYPE_FILE_MF,
    SWICC_FS_ITEM_TYPE_FILE_ADF,
    SWICC_FS_ITEM_TYPE_FILE_DF,
    SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT,
    SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED,
    SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC,
    // SWICC_FS_ITEM_TYPE_FILE_EF_LINEARVARIABLE,
    // SWICC_FS_ITEM_TYPE_FILE_EF_DATO,

    SWICC_FS_ITEM_TYPE_DATO_BERTLV,
    SWICC_FS_ITEM_TYPE_HEX,
    SWICC_FS_ITEM_TYPE_ASCII,
} swicc_fs_item_type_et;
static_assert(SWICC_FS_ITEM_TYPE_INVALID == 0U,
              "Invalid file type must be equal to 0");

/* Some helpers for checking the category of an item type. */
#define SWICC_FS_FILE_FOLDER_CHECK(file_p)                                     \
    (file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_MF ||                    \
     file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_DF ||                    \
     file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_ADF)
#define SWICC_FS_FILE_EF_CHECK(file_p)                                         \
    (file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT ||        \
     file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||        \
     file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
#define SWICC_FS_FILE_EF_BERTLV_CHECK(file_p) (false)
#define SWICC_FS_FILE_EF_BERTLV_NOT_CHECK(file_p)                              \
    (file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT ||        \
     file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||        \
     file_p->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)

/**
 * Life cycle status as specified in ISO 7816-4:2020 p.31 sec.7.4.10 table.15.
 */
typedef enum swicc_fs_lcs_e
{
    // SWICC_FS_LCS_NINFO,     /* No info given */
    // SWICC_FS_LCS_CREAT,     /* Creation */
    // SWICC_FS_LCS_INIT,      /* Initialization */
    SWICC_FS_LCS_OPER_ACTIV,   /* Operational + Activated */
    SWICC_FS_LCS_OPER_DEACTIV, /* Operational + Deactivated */
    SWICC_FS_LCS_TERM,         /* Termination */
} swicc_fs_lcs_et;

typedef enum swicc_fs_path_type_e
{
    SWICC_FS_PATH_TYPE_MF, /* Relative to the MF. */
    SWICC_FS_PATH_TYPE_DF, /* Relative to the current DF. */
} swicc_fs_path_type_et;

/**
 * Occurrence for specifying how a selection should be done if multiple items
 * match.
 */
typedef enum swicc_fs_occ_e
{
    SWICC_FS_OCC_FIRST,
    SWICC_FS_OCC_LAST,
    SWICC_FS_OCC_NEXT,
    SWICC_FS_OCC_PREV,
} swicc_fs_occ_et;

typedef uint16_t swicc_fs_id_kt; /* ID like FID. */
typedef uint8_t swicc_fs_sid_kt; /* Short ID like SFI. */
typedef uint8_t
    swicc_fs_rcrd_idx_kt; /* Record index (NOT the record number whose indexing
                             begins at 1, the IDX begins at 0). */

/**
 * A base header for any item in the swICC FS. The raw version is the header
 * stored in the in-memory swICC FS representation.
 */
typedef struct swicc_fs_item_hdr_s
{
    uint32_t size;

    /* Offset from top of the tree to the header of this item. */
    uint32_t offset_trel;

    /**
     * Offset from the start of the header of the parent to this item. A 0 means
     * the item has no parent.
     */
    uint32_t offset_prel;

    swicc_fs_item_type_et type;
    swicc_fs_lcs_et lcs;
} swicc_fs_item_hdr_st;
typedef struct swicc_fs_item_hdr_raw_s
{
    uint32_t size;
    uint32_t offset_prel;
    uint8_t type;
    uint8_t lcs;
} __attribute__((packed)) swicc_fs_item_hdr_raw_st;

/* Common header for all files (MF, EF, ADF, DF). */
typedef struct swicc_fs_file_hdr_s
{
    swicc_fs_id_kt id;
    swicc_fs_sid_kt sid;
} swicc_fs_file_hdr_st;
typedef struct swicc_fs_file_hdr_raw_s
{
    swicc_fs_id_kt id;
    swicc_fs_sid_kt sid;
} __attribute__((packed)) swicc_fs_file_hdr_raw_st;

/* Extra header data of an MF. */
typedef struct swicc_fs_mf_hdr_s
{
    uint8_t name[SWICC_FS_NAME_LEN];
} swicc_fs_mf_hdr_st;
typedef struct swicc_fs_mf_hdr_raw_s
{
    uint8_t name[SWICC_FS_NAME_LEN];
} __attribute__((packed)) swicc_fs_mf_hdr_raw_st;

/* Extra header data of a DF. */
typedef struct swicc_fs_df_hdr_s
{
    uint8_t name[SWICC_FS_NAME_LEN];
} swicc_fs_df_hdr_st;
typedef struct swicc_fs_df_hdr_raw_s
{
    uint8_t name[SWICC_FS_NAME_LEN];
} __attribute__((packed)) swicc_fs_df_hdr_raw_st;

/* Extra header data of an ADF. */
typedef struct swicc_fs_adf_hdr_s
{
    /**
     * This is for the Application IDentifier which is present ONLY for ADFs.
     * ETSI TS 101 220 v15.2.0 and ISO/IEC 7816-4:2020 12.3.4.
     */
    struct
    {
        /* Registered application provider IDentifier. */
        uint8_t rid[SWICC_FS_ADF_AID_RID_LEN];

        /* Proprietary application Identifier eXtension. */
        uint8_t pix[SWICC_FS_ADF_AID_PIX_LEN];
    } aid;
} swicc_fs_adf_hdr_st;
typedef struct swicc_fs_adf_hdr_raw_s
{
    struct
    {
        uint8_t rid[SWICC_FS_ADF_AID_RID_LEN];
        uint8_t pix[SWICC_FS_ADF_AID_PIX_LEN];
    } __attribute__((packed)) aid;
} __attribute__((packed)) swicc_fs_adf_hdr_raw_st;
static_assert(sizeof((swicc_fs_adf_hdr_raw_st){0U}.aid) == SWICC_FS_ADF_AID_LEN,
              "AID has an unexpected size in the raw header struct of ADF");

/* Extra header data of a transparent EF. */
typedef struct swicc_fs_ef_transparent_hdr_s
{
} swicc_fs_ef_transparent_hdr_st;
typedef struct swicc_fs_ef_transparent_hdr_raw_s
{
} __attribute__((packed)) swicc_fs_ef_transparent_hdr_raw_st;

/* Extra header data of a linear-fixed EF. */
typedef struct swicc_fs_ef_linearfixed_hdr_s
{
    uint8_t rcrd_size;
} swicc_fs_ef_linearfixed_hdr_st;
typedef struct swicc_fs_ef_linearfixed_hdr_raw_s
{
    uint8_t rcrd_size;
} __attribute__((packed)) swicc_fs_ef_linearfixed_hdr_raw_st;

/* Extra header data of a cyclic EF. */
typedef struct swicc_fs_ef_cyclic_hdr_s
{
    uint8_t rcrd_size;
} swicc_fs_ef_cyclic_hdr_st;
typedef struct swicc_fs_ef_cyclic_hdr_raw_s
{
    uint8_t rcrd_size;
} __attribute__((packed)) swicc_fs_ef_cyclic_hdr_raw_st;

/* Describes a record of an EF. */
typedef struct swicc_fs_rcrd_s
{
    swicc_fs_rcrd_idx_kt idx;
} swicc_fs_rcrd_st;

typedef struct swicc_fs_path_s
{
    swicc_fs_path_type_et type;
    swicc_fs_id_kt *b;
    uint32_t len;
} swicc_fs_path_st;

/**
 * Outside of the disk and diskjs modules, this shall be the struct that
 * abstracts away the implementation of files on disk.
 */
typedef struct swicc_fs_file_s
{
    swicc_fs_item_hdr_st hdr_item;
    swicc_fs_file_hdr_st hdr_file;
    union {
        swicc_fs_mf_hdr_st mf;
        swicc_fs_df_hdr_st df;
        swicc_fs_adf_hdr_st adf;
        swicc_fs_ef_transparent_hdr_st ef_transparent;
        swicc_fs_ef_linearfixed_hdr_st ef_linearfixed;
        swicc_fs_ef_cyclic_hdr_st ef_cyclic;
    } hdr_spec;
    uint32_t data_size;
    uint8_t *data;

    /* No module other than the FS should access this. */
    struct
    {
        uint8_t *hdr_raw;
    } internal;
} swicc_fs_file_st;
typedef struct swicc_fs_file_raw_s
{
    swicc_fs_item_hdr_raw_st hdr_item;
    swicc_fs_file_hdr_raw_st hdr_file;
    uint8_t data[]; /* If there is a spec header, it will be contained in the
                       data. */
} __attribute__((packed)) swicc_fs_file_raw_st;

extern uint32_t const swicc_fs_item_hdr_raw_size[];

/**
 * @brief Convert a raw item header to big endian.
 * @param[in, out] item_hdr_raw
 */
void swicc_fs_item_hdr_raw_be(swicc_fs_item_hdr_raw_st *const item_hdr_raw);

/**
 * @brief Convert a raw file header to big endian.
 * @param[in, out] file_hdr_raw
 */
void swicc_fs_file_hdr_raw_be(swicc_fs_file_hdr_raw_st *const file_hdr_raw);

/**
 * @brief Convert a raw MF header to big endian.
 * @param[in, out] mf_hdr_raw
 */
void swicc_fs_mf_hdr_raw_be(swicc_fs_mf_hdr_raw_st *const mf_hdr_raw);

/**
 * @brief Convert a raw DF header to big endian.
 * @param[in, out] df_hdr_raw
 */
void swicc_fs_df_hdr_raw_be(swicc_fs_df_hdr_raw_st *const df_hdr_raw);

/**
 * @brief Convert a raw transparent EF header to big endian.
 * @param[in, out] ef_transparent_hdr_raw
 */
void swicc_fs_ef_transparent_hdr_raw_be(
    swicc_fs_ef_transparent_hdr_raw_st *const ef_transparent_hdr_raw);

/**
 * @brief Convert a raw linear-fixed EF header to big endian.
 * @param[in, out] ef_linearfixed_hdr_raw
 */
void swicc_fs_ef_linearfixed_hdr_raw_be(
    swicc_fs_ef_linearfixed_hdr_raw_st *const ef_linearfixed_hdr_raw);

/**
 * @brief Convert a raw cyclic EF header to big endian.
 * @param[in, out] ef_cyclic_hdr_raw
 */
void swicc_fs_ef_cyclic_hdr_raw_be(
    swicc_fs_ef_cyclic_hdr_raw_st *const ef_cyclic_hdr_raw);

/**
 * @brief Convert a raw ADF header to big endian.
 * @param[in, out] adf_hdr_raw
 */
void swicc_fs_adf_hdr_raw_be(swicc_fs_adf_hdr_raw_st *const adf_hdr_raw);

/**
 * @brief Parse an item header.
 * @param[in] item_hdr_raw Pointer to the raw item header.
 * @param[in] offset_trel Tree-relative offset of the file.
 * @param[out] item_hdr Where to store the parsed item header.
 */
void swicc_fs_item_hdr_prs(swicc_fs_item_hdr_raw_st const *const item_hdr_raw,
                           uint32_t const offset_trel,
                           swicc_fs_item_hdr_st *const item_hdr);

/**
 * @brief Parse a file header.
 * @param[in] file_hdr_raw Pointer to the raw file header.
 * @param[out] file_hdr Where to store the parsed file header.
 */
void swicc_fs_file_hdr_prs(swicc_fs_file_hdr_raw_st const *const file_hdr_raw,
                           swicc_fs_file_hdr_st *const file_hdr);

/**
 * @brief Parse some location of the tree as a file.
 * @param[in] tree The tree containing the file.
 * @param[in] offset_trel Offset (inside the tree buffer) to the beginning of
 * the file.
 * @param[out] file Where the parsed file will be written.
 * @return Return code.
 */
swicc_ret_et swicc_fs_file_prs(swicc_disk_tree_st const *const tree,
                               uint32_t const offset_trel,
                               swicc_fs_file_st *const file);

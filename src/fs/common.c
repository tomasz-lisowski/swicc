#include <endian.h>
#include <string.h>
#include <uicc/uicc.h>

/**
 * Used for looking up header size of an item as it will be stored in the UICC
 * FS format.
 */
uint32_t const uicc_fs_item_hdr_raw_size[] = {
    [UICC_FS_ITEM_TYPE_INVALID] = 0U,
    [UICC_FS_ITEM_TYPE_FILE_MF] =
        sizeof(uicc_fs_file_raw_st) + sizeof(uicc_fs_mf_hdr_raw_st),
    [UICC_FS_ITEM_TYPE_FILE_ADF] =
        sizeof(uicc_fs_file_raw_st) + sizeof(uicc_fs_adf_hdr_raw_st),
    [UICC_FS_ITEM_TYPE_FILE_DF] =
        sizeof(uicc_fs_file_raw_st) + sizeof(uicc_fs_df_hdr_raw_st),
    [UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT] =
        sizeof(uicc_fs_file_raw_st) + sizeof(uicc_fs_ef_transparent_hdr_raw_st),
    [UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED] =
        sizeof(uicc_fs_file_raw_st) + sizeof(uicc_fs_ef_linearfixed_hdr_raw_st),
    [UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC] =
        sizeof(uicc_fs_file_raw_st) + sizeof(uicc_fs_ef_cyclic_hdr_raw_st),
    [UICC_FS_ITEM_TYPE_DATO_BERTLV] = 0U,
    [UICC_FS_ITEM_TYPE_HEX] = 0U,
    [UICC_FS_ITEM_TYPE_ASCII] = 0U,
};

void uicc_fs_item_hdr_raw_be(uicc_fs_item_hdr_raw_st *const item_hdr_raw)
{
    item_hdr_raw->offset_prel = htobe32(item_hdr_raw->offset_prel);
    item_hdr_raw->size = htobe32(item_hdr_raw->size);
}

void uicc_fs_file_hdr_raw_be(uicc_fs_file_hdr_raw_st *const file_hdr_raw)
{
    file_hdr_raw->id = htobe16(file_hdr_raw->id);
}

void uicc_fs_mf_hdr_raw_be(uicc_fs_mf_hdr_raw_st *const mf_hdr_raw)
{
}

void uicc_fs_df_hdr_raw_be(uicc_fs_df_hdr_raw_st *const df_hdr_raw)
{
}

void uicc_fs_ef_transparent_hdr_raw_be(
    uicc_fs_ef_transparent_hdr_raw_st *const ef_transparent_hdr_raw)
{
}

void uicc_fs_ef_linearfixed_hdr_raw_be(
    uicc_fs_ef_linearfixed_hdr_raw_st *const ef_linearfixed_hdr_raw)
{
}

void uicc_fs_ef_cyclic_hdr_raw_be(
    uicc_fs_ef_cyclic_hdr_raw_st *const ef_cyclic_hdr_raw)
{
}

void uicc_fs_adf_hdr_raw_be(uicc_fs_adf_hdr_raw_st *const adf_hdr_raw)
{
}

void uicc_fs_item_hdr_prs(uicc_fs_item_hdr_raw_st const *const item_hdr_raw,
                          uint32_t const offset_trel,
                          uicc_fs_item_hdr_st *const item_hdr)
{
    switch (item_hdr_raw->type)
    {
    case UICC_FS_ITEM_TYPE_FILE_MF:
    case UICC_FS_ITEM_TYPE_FILE_ADF:
    case UICC_FS_ITEM_TYPE_FILE_DF:
    case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
    case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
    case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
    case UICC_FS_ITEM_TYPE_DATO_BERTLV:
    case UICC_FS_ITEM_TYPE_HEX:
    case UICC_FS_ITEM_TYPE_ASCII:
        item_hdr->type = item_hdr_raw->type;
        break;
    case UICC_FS_ITEM_TYPE_INVALID:
        item_hdr->type = UICC_FS_ITEM_TYPE_INVALID;
    };
    item_hdr->lcs = item_hdr_raw->lcs;
    item_hdr->size = item_hdr_raw->size;
    item_hdr->offset_prel = item_hdr_raw->offset_prel;
    item_hdr->offset_trel = offset_trel;
}

void uicc_fs_file_hdr_prs(uicc_fs_file_hdr_raw_st const *const file_hdr_raw,
                          uicc_fs_file_hdr_st *const file_hdr)
{
    file_hdr->id = file_hdr_raw->id;
    file_hdr->sid = file_hdr_raw->sid;
}

uicc_ret_et uicc_fs_file_prs(uicc_disk_tree_st const *const tree,
                             uint32_t const offset_trel,
                             uicc_fs_file_st *const file)
{
    uicc_fs_item_hdr_raw_st const *const item_hdr_raw =
        (uicc_fs_item_hdr_raw_st *)&tree->buf[offset_trel];
    uicc_fs_file_hdr_raw_st const *const file_hdr_raw =
        (uicc_fs_file_hdr_raw_st *)&tree
            ->buf[offset_trel + sizeof(*item_hdr_raw)];
    uint64_t const offset_trel_hdr_spec =
        offset_trel + sizeof(*item_hdr_raw) + sizeof(*file_hdr_raw);
    if (offset_trel_hdr_spec >= UINT32_MAX)
    {
        return UICC_RET_ERROR;
    }

    memset(file, 0U, sizeof(*file));
    uicc_fs_item_hdr_prs(item_hdr_raw, offset_trel, &file->hdr_item);
    uicc_fs_file_hdr_prs(file_hdr_raw, &file->hdr_file);
    file->internal.hdr_raw = &tree->buf[offset_trel];

    switch (file->hdr_item.type)
    {
    case UICC_FS_ITEM_TYPE_FILE_MF: {
        uicc_fs_mf_hdr_raw_st const *const mf_hdr_raw =
            (uicc_fs_mf_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        memcpy(file->hdr_spec.mf.name, mf_hdr_raw->name, UICC_FS_NAME_LEN);
        break;
    }
    case UICC_FS_ITEM_TYPE_FILE_ADF: {
        uicc_fs_adf_hdr_raw_st const *const adf_hdr_raw =
            (uicc_fs_adf_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        memcpy(file->hdr_spec.adf.aid.rid, adf_hdr_raw->aid.rid,
               sizeof(file->hdr_spec.adf.aid.rid));
        memcpy(file->hdr_spec.adf.aid.pix, adf_hdr_raw->aid.pix,
               sizeof(file->hdr_spec.adf.aid.pix));
        break;
    }
    case UICC_FS_ITEM_TYPE_FILE_DF: {
        uicc_fs_df_hdr_raw_st const *const df_hdr_raw =
            (uicc_fs_df_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        memcpy(file->hdr_spec.df.name, df_hdr_raw->name, UICC_FS_NAME_LEN);
        break;
    }
    case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT: {
        static_assert(sizeof(uicc_fs_ef_transparent_hdr_raw_st) == 0,
                      "Transparent EF header is not 0 bytes so need to add "
                      "parsing for extra fields into the file parser");
        break;
    }
    case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED: {
        uicc_fs_ef_linearfixed_hdr_raw_st const *const ef_linearfixed_hdr_raw =
            (uicc_fs_ef_linearfixed_hdr_raw_st *)&tree
                ->buf[offset_trel_hdr_spec];
        file->hdr_spec.ef_linearfixed.rcrd_size =
            ef_linearfixed_hdr_raw->rcrd_size;
        break;
    }
    case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC: {
        uicc_fs_ef_cyclic_hdr_raw_st const *const ef_cyclic_hdr_raw =
            (uicc_fs_ef_cyclic_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        file->hdr_spec.ef_cyclic.rcrd_size = ef_cyclic_hdr_raw->rcrd_size;
        break;
    }
    /* For handling anything that is not a valid file type. */
    default:
        return UICC_RET_ERROR;
    }

    uint32_t const hdr_size = uicc_fs_item_hdr_raw_size[file->hdr_item.type];
    file->data_size = file->hdr_item.size - hdr_size;
    file->data = &tree->buf[offset_trel + hdr_size];

    return UICC_RET_SUCCESS;
}

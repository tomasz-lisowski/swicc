#include <endian.h>
#include <string.h>
#include <swicc/swicc.h>

/**
 * Used for looking up header size of an item as it will be stored in the swICC
 * FS disk format.
 */
uint32_t const swicc_fs_item_hdr_raw_size[] = {
    [SWICC_FS_ITEM_TYPE_INVALID] = 0U,
    [SWICC_FS_ITEM_TYPE_FILE_MF] =
        sizeof(swicc_fs_file_raw_st) + sizeof(swicc_fs_mf_hdr_raw_st),
    [SWICC_FS_ITEM_TYPE_FILE_ADF] =
        sizeof(swicc_fs_file_raw_st) + sizeof(swicc_fs_adf_hdr_raw_st),
    [SWICC_FS_ITEM_TYPE_FILE_DF] =
        sizeof(swicc_fs_file_raw_st) + sizeof(swicc_fs_df_hdr_raw_st),
    [SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT] =
        sizeof(swicc_fs_file_raw_st) +
        sizeof(swicc_fs_ef_transparent_hdr_raw_st),
    [SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED] =
        sizeof(swicc_fs_file_raw_st) +
        sizeof(swicc_fs_ef_linearfixed_hdr_raw_st),
    [SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC] =
        sizeof(swicc_fs_file_raw_st) + sizeof(swicc_fs_ef_cyclic_hdr_raw_st),
    [SWICC_FS_ITEM_TYPE_DATO_BERTLV] = 0U,
    [SWICC_FS_ITEM_TYPE_HEX] = 0U,
    [SWICC_FS_ITEM_TYPE_ASCII] = 0U,
};

void swicc_fs_item_hdr_raw_be(swicc_fs_item_hdr_raw_st *const item_hdr_raw)
{
    item_hdr_raw->offset_prel = htobe32(item_hdr_raw->offset_prel);
    item_hdr_raw->size = htobe32(item_hdr_raw->size);
}

void swicc_fs_file_hdr_raw_be(swicc_fs_file_hdr_raw_st *const file_hdr_raw)
{
    file_hdr_raw->id = htobe16(file_hdr_raw->id);
}

void swicc_fs_mf_hdr_raw_be(swicc_fs_mf_hdr_raw_st *const mf_hdr_raw)
{
}

void swicc_fs_df_hdr_raw_be(swicc_fs_df_hdr_raw_st *const df_hdr_raw)
{
}

void swicc_fs_ef_transparent_hdr_raw_be(
    swicc_fs_ef_transparent_hdr_raw_st *const ef_transparent_hdr_raw)
{
}

void swicc_fs_ef_linearfixed_hdr_raw_be(
    swicc_fs_ef_linearfixed_hdr_raw_st *const ef_linearfixed_hdr_raw)
{
}

void swicc_fs_ef_cyclic_hdr_raw_be(
    swicc_fs_ef_cyclic_hdr_raw_st *const ef_cyclic_hdr_raw)
{
}

void swicc_fs_adf_hdr_raw_be(swicc_fs_adf_hdr_raw_st *const adf_hdr_raw)
{
}

void swicc_fs_item_hdr_prs(swicc_fs_item_hdr_raw_st const *const item_hdr_raw,
                           uint32_t const offset_trel,
                           swicc_fs_item_hdr_st *const item_hdr)
{
    switch (item_hdr_raw->type)
    {
    case SWICC_FS_ITEM_TYPE_FILE_MF:
    case SWICC_FS_ITEM_TYPE_FILE_ADF:
    case SWICC_FS_ITEM_TYPE_FILE_DF:
    case SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
    case SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
    case SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
    case SWICC_FS_ITEM_TYPE_DATO_BERTLV:
    case SWICC_FS_ITEM_TYPE_HEX:
    case SWICC_FS_ITEM_TYPE_ASCII:
        item_hdr->type = item_hdr_raw->type;
        break;
    case SWICC_FS_ITEM_TYPE_INVALID:
        item_hdr->type = SWICC_FS_ITEM_TYPE_INVALID;
    };
    item_hdr->lcs = item_hdr_raw->lcs;
    item_hdr->size = item_hdr_raw->size;
    item_hdr->offset_prel = item_hdr_raw->offset_prel;
    item_hdr->offset_trel = offset_trel;
}

void swicc_fs_file_hdr_prs(swicc_fs_file_hdr_raw_st const *const file_hdr_raw,
                           swicc_fs_file_hdr_st *const file_hdr)
{
    file_hdr->id = file_hdr_raw->id;
    file_hdr->sid = file_hdr_raw->sid;
}

swicc_ret_et swicc_fs_file_prs(swicc_disk_tree_st const *const tree,
                               uint32_t const offset_trel,
                               swicc_fs_file_st *const file)
{
    swicc_fs_item_hdr_raw_st const *const item_hdr_raw =
        (swicc_fs_item_hdr_raw_st *)&tree->buf[offset_trel];
    swicc_fs_file_hdr_raw_st const *const file_hdr_raw =
        (swicc_fs_file_hdr_raw_st *)&tree
            ->buf[offset_trel + sizeof(*item_hdr_raw)];
    uint64_t const offset_trel_hdr_spec =
        offset_trel + sizeof(*item_hdr_raw) + sizeof(*file_hdr_raw);
    if (offset_trel_hdr_spec >= UINT32_MAX)
    {
        return SWICC_RET_ERROR;
    }

    memset(file, 0U, sizeof(*file));
    swicc_fs_item_hdr_prs(item_hdr_raw, offset_trel, &file->hdr_item);
    swicc_fs_file_hdr_prs(file_hdr_raw, &file->hdr_file);
    file->internal.hdr_raw = &tree->buf[offset_trel];

    switch (file->hdr_item.type)
    {
    case SWICC_FS_ITEM_TYPE_FILE_MF: {
        swicc_fs_mf_hdr_raw_st const *const mf_hdr_raw =
            (swicc_fs_mf_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        memcpy(file->hdr_spec.mf.name, mf_hdr_raw->name, SWICC_FS_NAME_LEN);
        break;
    }
    case SWICC_FS_ITEM_TYPE_FILE_ADF: {
        swicc_fs_adf_hdr_raw_st const *const adf_hdr_raw =
            (swicc_fs_adf_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        memcpy(file->hdr_spec.adf.aid.rid, adf_hdr_raw->aid.rid,
               sizeof(file->hdr_spec.adf.aid.rid));
        memcpy(file->hdr_spec.adf.aid.pix, adf_hdr_raw->aid.pix,
               sizeof(file->hdr_spec.adf.aid.pix));
        break;
    }
    case SWICC_FS_ITEM_TYPE_FILE_DF: {
        swicc_fs_df_hdr_raw_st const *const df_hdr_raw =
            (swicc_fs_df_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        memcpy(file->hdr_spec.df.name, df_hdr_raw->name, SWICC_FS_NAME_LEN);
        break;
    }
    case SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT: {
        static_assert(sizeof(swicc_fs_ef_transparent_hdr_raw_st) == 0,
                      "Transparent EF header is not 0 bytes so need to add "
                      "parsing for extra fields into the file parser");
        break;
    }
    case SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED: {
        swicc_fs_ef_linearfixed_hdr_raw_st const *const ef_linearfixed_hdr_raw =
            (swicc_fs_ef_linearfixed_hdr_raw_st *)&tree
                ->buf[offset_trel_hdr_spec];
        file->hdr_spec.ef_linearfixed.rcrd_size =
            ef_linearfixed_hdr_raw->rcrd_size;
        break;
    }
    case SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC: {
        swicc_fs_ef_cyclic_hdr_raw_st const *const ef_cyclic_hdr_raw =
            (swicc_fs_ef_cyclic_hdr_raw_st *)&tree->buf[offset_trel_hdr_spec];
        file->hdr_spec.ef_cyclic.rcrd_size = ef_cyclic_hdr_raw->rcrd_size;
        break;
    }
    /* For handling anything that is not a valid file type. */
    default:
        return SWICC_RET_ERROR;
    }

    uint32_t const hdr_size = swicc_fs_item_hdr_raw_size[file->hdr_item.type];
    file->data_size = file->hdr_item.size - hdr_size;
    file->data = &tree->buf[offset_trel + hdr_size];

    return SWICC_RET_SUCCESS;
}

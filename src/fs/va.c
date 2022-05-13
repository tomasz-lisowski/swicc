#include "uicc.h"
#include <string.h>

uicc_ret_et uicc_va_reset(uicc_fs_st *const fs)
{
    memset(&fs->va, 0U, sizeof(fs->va));
    uicc_disk_tree_iter_st tree_iter;
    uicc_ret_et ret = uicc_disk_tree_iter(&fs->disk, &tree_iter);
    if (ret == UICC_RET_SUCCESS)
    {
        ret = uicc_va_select_file_id(fs, 0x003F);
        if (ret == UICC_RET_SUCCESS)
        {
            return ret;
        }
    }
    return ret;
}

uicc_ret_et uicc_va_select_adf(uicc_fs_st *const fs, uint8_t const *const aid,
                               uint32_t const pix_len)
{
    uicc_disk_tree_iter_st tree_iter;
    uicc_ret_et ret = uicc_disk_tree_iter(&fs->disk, &tree_iter);
    if (ret == UICC_RET_SUCCESS)
    {
        uicc_disk_tree_st *tree;
        do
        {
            ret = uicc_disk_tree_iter_next(&tree_iter, &tree);
            if (ret == UICC_RET_SUCCESS)
            {
                uicc_fs_adf_hdr_st adf_hdr;
                uicc_fs_file_hdr_raw_st *file_hdr_raw;
                uicc_fs_adf_hdr_raw_st *adf_hdr_raw;
                uint32_t adf_offset_trel;
                ret =
                    uicc_disk_tree_file(tree, &file_hdr_raw, &adf_offset_trel);
                if (ret == UICC_RET_SUCCESS)
                {
                    ret = uicc_fs_file_hdr_prs(file_hdr_raw, &adf_hdr.file);
                    if (ret == UICC_RET_SUCCESS)
                    {
                        ret = uicc_fs_item_hdr_prs(&file_hdr_raw->item,
                                                   &adf_hdr.file.item);
                        if (ret == UICC_RET_SUCCESS)
                        {
                            if (adf_hdr.file.item.type ==
                                UICC_FS_ITEM_TYPE_FILE_ADF)
                            {
                                adf_hdr_raw =
                                    (uicc_fs_adf_hdr_raw_st *)file_hdr_raw;
                                memcpy(adf_hdr.aid.rid, adf_hdr_raw->aid.rid,
                                       sizeof(adf_hdr_raw->aid.rid));
                                memcpy(adf_hdr.aid.pix, adf_hdr_raw->aid.pix,
                                       sizeof(adf_hdr_raw->aid.pix));
                                if (memcmp(adf_hdr.aid.rid, aid,
                                           UICC_FS_ADF_AID_RID_LEN) == 0U &&
                                    memcmp(adf_hdr.aid.pix,
                                           &aid[UICC_FS_ADF_AID_RID_LEN],
                                           pix_len) == 0U)
                                {
                                    /* Got a match. */
                                    adf_hdr.file.item.offset_trel =
                                        adf_offset_trel;
                                    memset(&fs->va, 0U, sizeof(fs->va));
                                    fs->va.cur_adf = tree;
                                    fs->va.cur_df = adf_hdr.file;
                                    return UICC_RET_SUCCESS;
                                }
                            }
                        }
                    }
                }
            }
        } while (ret == UICC_RET_SUCCESS);
    }
    return ret;
}

uicc_ret_et uicc_va_select_file_dfname(uicc_fs_st *const fs,
                                       char const *const df_name,
                                       uint32_t const df_name_len)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_va_select_file_id(uicc_fs_st *const fs,
                                   uicc_fs_id_kt const fid)
{
    uicc_disk_lut_st *const lutid = &fs->disk.lutid;

    /* Make sure the ID LUT is as expected. */
    if (lutid->buf1 == NULL || lutid->size_item1 != sizeof(uicc_fs_id_kt) ||
        lutid->size_item2 != sizeof(uint32_t) + sizeof(uint8_t))
    {
        return UICC_RET_ERROR;
    }

    /* Find the file by ID. */
    uint32_t entry_idx = 0U;
    while (entry_idx < lutid->count)
    {
        if (memcmp(&lutid->buf1[lutid->size_item1 * entry_idx], &fid,
                   lutid->size_item1) == 0)
        {
            break;
        }
        entry_idx += 1U;
    }
    if (entry_idx >= lutid->count)
    {
        return UICC_RET_FS_NOT_FOUND;
    }
    else
    {
        uint32_t const offset =
            *(uint32_t *)&lutid->buf2[(lutid->size_item2 * entry_idx) + 0U];
        uint8_t const tree_idx =
            lutid->buf2[(lutid->size_item2 * entry_idx) + sizeof(uint32_t)];

        uicc_disk_tree_st *tree;
        uicc_disk_tree_iter_st tree_iter;
        if (uicc_disk_tree_iter(&fs->disk, &tree_iter) != UICC_RET_SUCCESS ||
            uicc_disk_tree_iter_idx(&tree_iter, tree_idx, &tree) !=
                UICC_RET_SUCCESS ||
            tree->len <= offset)
        {
            return UICC_RET_ERROR;
        }

        uicc_fs_file_hdr_st file;
        uicc_fs_file_hdr_raw_st *const file_raw =
            (uicc_fs_file_hdr_raw_st *)&tree->buf[offset];
        if (uicc_fs_item_hdr_prs(&file_raw->item, &file.item) !=
            UICC_RET_SUCCESS)
        {
            return UICC_RET_ERROR;
        }
        file.item.offset_trel = offset;

        /**
         * Rules for modifying the VA are described in ISO 7816-4:2020 p.22
         * sec.7.2.2.
         */

        switch (file.item.type)
        {
        case UICC_FS_ITEM_TYPE_FILE_MF:
        case UICC_FS_ITEM_TYPE_FILE_ADF:
        case UICC_FS_ITEM_TYPE_FILE_DF:
            memset(&fs->va, 0U, sizeof(fs->va));
            fs->va.cur_df = file;
            break;
        case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
        case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
        case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC: {
            uicc_fs_file_hdr_st file_parent;
            uicc_fs_file_hdr_raw_st *const file_parent_raw =
                (uicc_fs_file_hdr_raw_st *)&tree
                    ->buf[file.item.offset_trel - file.item.offset_prel];
            if (uicc_fs_item_hdr_prs(&file_parent_raw->item,
                                     &file_parent.item) != UICC_RET_SUCCESS ||
                !(file_parent.item.type == UICC_FS_ITEM_TYPE_FILE_MF ||
                  file_parent.item.type == UICC_FS_ITEM_TYPE_FILE_ADF ||
                  file_parent.item.type == UICC_FS_ITEM_TYPE_FILE_DF))
            {
                /* Parent must be a folder. */
                return UICC_RET_ERROR;
            }
            memset(&fs->va, 0U, sizeof(fs->va));
            fs->va.cur_df = file_parent;
            fs->va.cur_ef = file;
            break;
        }
        default:
            return UICC_RET_ERROR;
        }
        fs->va.cur_adf = tree;
        return UICC_RET_SUCCESS;
    }
}

uicc_ret_et uicc_va_select_file_path(uicc_fs_st *const fs,
                                     uicc_fs_path_st const path)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_va_select_do_tag(uicc_fs_st *const fs, uint16_t const tag)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_va_select_record_id(uicc_fs_st *const fs,
                                     uicc_fs_rcrd_id_kt id)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_va_select_record_idx(uicc_fs_st *const fs,
                                      uicc_fs_rcrd_idx_kt idx)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_va_select_data_offset(uicc_fs_st *const fs,
                                       uint32_t offset_prel)
{
    return UICC_RET_UNKNOWN;
}

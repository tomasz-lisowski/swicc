#include "uicc/fs/common.h"
#include <string.h>
#include <uicc/uicc.h>

/**
 * @brief Helper for performing file selection according to the standard. Rules
 * for modifying the VA are described in ISO 7816-4:2020 p.22 sec.7.2.2.
 * @param fs
 * @param tree This tree must contain the file.
 * @param file File to select.
 * @return Return code.
 */
static uicc_ret_et va_select_file(uicc_fs_st *const fs,
                                  uicc_disk_tree_st *const tree,
                                  uicc_fs_file_st const file)
{
    uicc_fs_file_st file_root;
    uicc_ret_et ret = uicc_disk_tree_file_root(tree, &file_root);
    if (ret == UICC_RET_SUCCESS)
    {
        uicc_fs_file_st file_parent;
        ret = uicc_disk_tree_file_parent(tree, &file, &file_parent);
        if (ret == UICC_RET_SUCCESS)
        {
            switch (file.hdr_item.type)
            {
            case UICC_FS_ITEM_TYPE_FILE_MF:
            case UICC_FS_ITEM_TYPE_FILE_ADF:
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                fs->va.cur_adf = file;
                fs->va.cur_df = file;
                fs->va.cur_file = file;
                break;
            case UICC_FS_ITEM_TYPE_FILE_DF:
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                fs->va.cur_adf = file_root;
                fs->va.cur_df = file;
                fs->va.cur_file = file;
                break;
            case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
            case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
            case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
                /**
                 * @warning ISO 7816-4:2020 pg.23 sec.7.2.2 states that "When EF
                 * selection occurs as a side-effect of a C-RP using referencing
                 * by short EF identifier, curEF may change, while curDF does
                 * not change" but in this implementation, current DF always
                 * changes even for selections using SID.
                 */
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                fs->va.cur_adf = file_root;
                fs->va.cur_df = file_parent;
                fs->va.cur_ef = file;
                fs->va.cur_file = file;
                break;
            default:
                return UICC_RET_ERROR;
            }
        }
    }
    return ret;
}

uicc_ret_et uicc_va_reset(uicc_fs_st *const fs)
{
    memset(&fs->va, 0U, sizeof(fs->va));
    uicc_disk_tree_iter_st tree_iter;
    uicc_ret_et ret = uicc_disk_tree_iter(&fs->disk, &tree_iter);
    if (ret == UICC_RET_SUCCESS)
    {
        ret = uicc_va_select_file_id(fs, 0x3F00);
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
                uicc_fs_file_st file_root;
                ret = uicc_disk_tree_file_root(tree, &file_root);
                if (ret == UICC_RET_SUCCESS)
                {
                    if (file_root.hdr_item.type == UICC_FS_ITEM_TYPE_FILE_ADF)
                    {
                        if (memcmp(file_root.hdr_spec.adf.aid.rid, aid,
                                   UICC_FS_ADF_AID_RID_LEN) == 0U &&
                            memcmp(file_root.hdr_spec.adf.aid.pix,
                                   &aid[UICC_FS_ADF_AID_RID_LEN],
                                   pix_len) == 0U)
                        {
                            return va_select_file(fs, tree, file_root);
                        }
                    }
                    else
                    {
                        ret = UICC_RET_ERROR;
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
    uicc_disk_tree_st *tree;
    uicc_fs_file_st file;
    uicc_ret_et ret = uicc_disk_lutid_lookup(&fs->disk, &tree, fid, &file);
    if (ret == UICC_RET_SUCCESS)
    {
        return va_select_file(fs, tree, file);
    }
    return ret;
}

uicc_ret_et uicc_va_select_file_sid(uicc_fs_st *const fs,
                                    uicc_fs_sid_kt const sid)
{
    uicc_fs_file_st file;
    uicc_ret_et const ret =
        uicc_disk_lutsid_lookup(fs->va.cur_tree, sid, &file);
    if (ret == UICC_RET_SUCCESS)
    {
        return va_select_file(fs, fs->va.cur_tree, file);
    }
    return ret;
}

uicc_ret_et uicc_va_select_file_path(uicc_fs_st *const fs,
                                     uicc_fs_path_st const path)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_va_select_record_idx(uicc_fs_st *const fs,
                                      uicc_fs_rcrd_idx_kt idx)
{
    if (fs->va.cur_ef.hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||
        fs->va.cur_ef.hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        uint32_t rcrd_cnt;
        if (uicc_disk_file_rcrd_cnt(fs->va.cur_tree, &fs->va.cur_ef,
                                    &rcrd_cnt) == UICC_RET_SUCCESS)
        {
            uicc_fs_rcrd_st const rcrd = {.idx = idx};
            fs->va.cur_rcrd = rcrd;
            return UICC_RET_SUCCESS;
        }
    }
    return UICC_RET_ERROR;
}

uicc_ret_et uicc_va_select_data_offset(uicc_fs_st *const fs,
                                       uint32_t offset_prel)
{
    return UICC_RET_UNKNOWN;
}

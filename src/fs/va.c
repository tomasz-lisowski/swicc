#include "swicc/fs/common.h"
#include <string.h>
#include <swicc/swicc.h>

/**
 * @brief Helper for performing file selection according to the standard. Rules
 * for modifying the VA are described in ISO 7816-4:2020 p.22 sec.7.2.2.
 * @param fs
 * @param tree This tree must contain the file.
 * @param file File to select.
 * @return Return code.
 */
static swicc_ret_et va_select_file(swicc_fs_st *const fs,
                                   swicc_disk_tree_st *const tree,
                                   swicc_fs_file_st const file)
{
    swicc_fs_file_st file_root;
    swicc_ret_et ret = swicc_disk_tree_file_root(tree, &file_root);
    if (ret == SWICC_RET_SUCCESS)
    {
        swicc_fs_file_st file_parent;
        ret = swicc_disk_tree_file_parent(tree, &file, &file_parent);
        if (ret == SWICC_RET_SUCCESS)
        {
            switch (file.hdr_item.type)
            {
            case SWICC_FS_ITEM_TYPE_FILE_MF: {
                swicc_fs_file_st const file_adf = fs->va.cur_adf;
                swicc_disk_tree_st *const tree_adf = fs->va.cur_tree_adf;
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                fs->va.cur_adf = file_adf;
                fs->va.cur_tree_adf = tree_adf;
                fs->va.cur_df = file;
                fs->va.cur_file = file;
                break;
            }
            case SWICC_FS_ITEM_TYPE_FILE_ADF:
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                fs->va.cur_adf = file;
                fs->va.cur_tree_adf = tree;
                fs->va.cur_df = file;
                fs->va.cur_file = file;
                break;
            case SWICC_FS_ITEM_TYPE_FILE_DF: {
                swicc_fs_file_st const file_adf = fs->va.cur_adf;
                swicc_disk_tree_st *const tree_adf = fs->va.cur_tree_adf;
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                if (file_root.hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_ADF)
                {
                    fs->va.cur_adf = file_root;
                    fs->va.cur_tree_adf = tree;
                }
                else
                {
                    fs->va.cur_adf = file_adf;
                    fs->va.cur_tree_adf = tree_adf;
                }
                fs->va.cur_df = file;
                fs->va.cur_file = file;
                break;
            }
            case SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
            case SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
            case SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC: {
                /**
                 * @warning ISO 7816-4:2020 pg.23 sec.7.2.2 states that "When EF
                 * selection occurs as a side-effect of a C-RP using referencing
                 * by short EF identifier, curEF may change, while curDF does
                 * not change" but in this implementation, current DF always
                 * changes even for selections using SID.
                 */
                swicc_fs_file_st const file_adf = fs->va.cur_adf;
                swicc_disk_tree_st *const tree_adf = fs->va.cur_tree_adf;
                memset(&fs->va, 0U, sizeof(fs->va));
                fs->va.cur_tree = tree;
                if (file_root.hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_ADF)
                {
                    fs->va.cur_adf = file_root;
                    fs->va.cur_tree_adf = tree;
                }
                else
                {
                    fs->va.cur_adf = file_adf;
                    fs->va.cur_tree_adf = tree_adf;
                }
                fs->va.cur_df = file_parent;
                fs->va.cur_ef = file;
                fs->va.cur_file = file;
                break;
            }
            default:
                return SWICC_RET_FS_NOT_FOUND;
            }
        }
    }
    return ret;
}

swicc_ret_et swicc_va_reset(swicc_fs_st *const fs)
{
    memset(&fs->va, 0U, sizeof(fs->va));
    swicc_disk_tree_iter_st tree_iter;
    swicc_ret_et ret = swicc_disk_tree_iter(&fs->disk, &tree_iter);
    if (ret == SWICC_RET_SUCCESS)
    {
        ret = swicc_va_select_file_id(fs, 0x3F00);
        if (ret == SWICC_RET_SUCCESS)
        {
            return ret;
        }
    }
    return ret;
}

swicc_ret_et swicc_va_select_adf(swicc_fs_st *const fs,
                                 uint8_t const *const aid,
                                 uint32_t const pix_len)
{
    swicc_disk_tree_iter_st tree_iter;
    swicc_ret_et ret = swicc_disk_tree_iter(&fs->disk, &tree_iter);
    if (ret == SWICC_RET_SUCCESS)
    {
        swicc_disk_tree_st *tree;
        do
        {
            ret = swicc_disk_tree_iter_next(&tree_iter, &tree);
            if (ret == SWICC_RET_SUCCESS)
            {
                swicc_fs_file_st file_root;
                ret = swicc_disk_tree_file_root(tree, &file_root);
                if (ret == SWICC_RET_SUCCESS)
                {
                    if (file_root.hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_ADF)
                    {
                        if (memcmp(file_root.hdr_spec.adf.aid.rid, aid,
                                   SWICC_FS_ADF_AID_RID_LEN) == 0U &&
                            memcmp(file_root.hdr_spec.adf.aid.pix,
                                   &aid[SWICC_FS_ADF_AID_RID_LEN],
                                   pix_len) == 0U)
                        {
                            return va_select_file(fs, tree, file_root);
                        }
                    }
                    else
                    {
                        ret = SWICC_RET_ERROR;
                    }
                }
            }
        } while (ret == SWICC_RET_SUCCESS);
    }
    return ret;
}

typedef struct va_select_file_dfname_userdata_s
{
    uint8_t const name[SWICC_FS_NAME_LEN];
    uint32_t const name_len;
    bool found;
    swicc_fs_file_st file_found;
} va_select_file_dfname_userdata_st;
static swicc_disk_file_foreach_cb va_select_file_dfname_cb;
static swicc_ret_et va_select_file_dfname_cb(swicc_disk_tree_st *const tree,
                                             swicc_fs_file_st *const file,
                                             void *const userdata)
{
    va_select_file_dfname_userdata_st *const ud = userdata;
    uint8_t const *name = NULL;
    switch (file->hdr_item.type)
    {
    case SWICC_FS_ITEM_TYPE_FILE_MF:
        name = file->hdr_spec.mf.name;
        break;
    case SWICC_FS_ITEM_TYPE_FILE_DF:
        name = file->hdr_spec.df.name;
        break;
    default:
        break;
    }
    if (name != NULL)
    {
        if (ud->name_len > SWICC_FS_NAME_LEN)
        {
            return SWICC_RET_PARAM_BAD;
        }
        if (memcmp(name, ud->name, ud->name_len) == 0)
        {
            ud->found = true;
            ud->file_found = *file;
            /* Make the foreach iterator stop early. */
            return SWICC_RET_ERROR;
        }
    }
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_va_select_file_dfname(swicc_fs_st *const fs,
                                         uint8_t const *const df_name,
                                         uint32_t const df_name_len)
{
    va_select_file_dfname_userdata_st userdata = {
        .name = {*df_name},
        .name_len = df_name_len,
        .found = false,
    };
    swicc_disk_tree_iter_st tree_iter;
    swicc_ret_et ret = swicc_disk_tree_iter(&fs->disk, &tree_iter);
    if (ret == SWICC_RET_SUCCESS)
    {
        swicc_disk_tree_st *tree;
        do
        {
            ret = swicc_disk_tree_iter_next(&tree_iter, &tree);
            if (ret == SWICC_RET_SUCCESS)
            {
                swicc_fs_file_st file_root;
                ret = swicc_disk_tree_file_root(tree, &file_root);
                if (ret == SWICC_RET_SUCCESS)
                {
                    ret = swicc_disk_file_foreach(tree, &file_root,
                                                  va_select_file_dfname_cb,
                                                  &userdata, true);
                    if (ret == SWICC_RET_SUCCESS)
                    {
                        /**
                         * No DF/MF with the name was found and foreach did not
                         * fail.
                         */
                        ret = SWICC_RET_FS_NOT_FOUND;
                        continue;
                    }
                    else
                    {
                        if (userdata.found == true)
                        {
                            ret = va_select_file(fs, tree, userdata.file_found);
                            if (ret == SWICC_RET_SUCCESS)
                            {
                                return SWICC_RET_SUCCESS;
                            }
                        }
                        /* Stop looking on any non-successful return. */
                        return ret;
                    }
                }
            }
        } while (ret == SWICC_RET_SUCCESS);
    }
    return ret;
}

swicc_ret_et swicc_va_select_file_id(swicc_fs_st *const fs,
                                     swicc_fs_id_kt const fid)
{
    swicc_disk_tree_st *tree;
    swicc_fs_file_st file;
    swicc_ret_et ret = swicc_disk_lutid_lookup(&fs->disk, &tree, fid, &file);
    if (ret == SWICC_RET_SUCCESS)
    {
        return va_select_file(fs, tree, file);
    }
    return ret;
}

swicc_ret_et swicc_va_select_file_sid(swicc_fs_st *const fs,
                                      swicc_fs_sid_kt const sid)
{
    swicc_fs_file_st file;
    swicc_ret_et const ret =
        swicc_disk_lutsid_lookup(fs->va.cur_tree, sid, &file);
    if (ret == SWICC_RET_SUCCESS)
    {
        return va_select_file(fs, fs->va.cur_tree, file);
    }
    return ret;
}

typedef struct va_select_file_path_userdata_s
{
    swicc_fs_id_kt fid_search;
    bool found;
    swicc_fs_file_st file_found;
} va_select_file_path_userdata_st;
static swicc_disk_file_foreach_cb va_select_file_path_cb;
static swicc_ret_et va_select_file_path_cb(swicc_disk_tree_st *const tree,
                                           swicc_fs_file_st *const file,
                                           void *const userdata)
{
    va_select_file_path_userdata_st *const ud = userdata;
    if (file->hdr_file.id == ud->fid_search)
    {
        ud->found = true;
        ud->file_found = *file;
        return SWICC_RET_ERROR;
    }
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_va_select_file_path(swicc_fs_st *const fs,
                                       swicc_fs_path_st const path)
{
    swicc_ret_et ret = SWICC_RET_ERROR;
    swicc_fs_file_st file_root;
    swicc_disk_tree_st *tree = NULL;
    va_select_file_path_userdata_st userdata = {0U};

    switch (path.type)
    {
    case SWICC_FS_PATH_TYPE_MF:
        /* Reserved FID 0x7FFF refers to the current application. */
        if (path.b[0U] == 0x7FFF)
        {
            if (fs->va.cur_adf.hdr_item.type != SWICC_FS_ITEM_TYPE_FILE_ADF ||
                fs->va.cur_tree_adf == NULL)
            {
                return SWICC_RET_FS_NOT_FOUND;
            }
            tree = fs->va.cur_tree_adf;
            ret = swicc_disk_lutid_lookup(
                &fs->disk, &tree, fs->va.cur_adf.hdr_file.id, &file_root);
            if (ret != SWICC_RET_SUCCESS)
            {
                return ret;
            }
            path.b[0U] = fs->va.cur_adf.hdr_file.id;
        }
        else
        {
            tree = fs->disk.root;
            ret = swicc_disk_lutid_lookup(&fs->disk, &tree, 0x3F00, &file_root);
            if (ret != SWICC_RET_SUCCESS)
            {
                return ret;
            }
        }
        break;
    case SWICC_FS_PATH_TYPE_DF:
        tree = fs->va.cur_tree;
        file_root = fs->va.cur_df;
        break;
    }

    /* Traverse path. */
    for (uint32_t path_idx = 0U; path_idx < path.len; ++path_idx)
    {
        swicc_fs_id_kt fid_next = path.b[path_idx];
        userdata.fid_search = fid_next;
        userdata.found = false;
        ret = swicc_disk_file_foreach(tree, &file_root, va_select_file_path_cb,
                                      &userdata, false);
        if (ret == SWICC_RET_ERROR && userdata.found == true)
        {
            file_root = userdata.file_found;
            ret = SWICC_RET_SUCCESS;
        }
        else if (ret == SWICC_RET_SUCCESS)
        {
            return SWICC_RET_FS_NOT_FOUND;
        }
        else
        {
            return ret;
        }
    }

    /* After traversing path, try select it. */
    if (ret == SWICC_RET_SUCCESS && userdata.found && tree != NULL)
    {
        ret = va_select_file(fs, tree, userdata.file_found);
        return ret;
    }
    return ret;
}

swicc_ret_et swicc_va_select_record_idx(swicc_fs_st *const fs,
                                        swicc_fs_rcrd_idx_kt idx)
{
    if (fs->va.cur_ef.hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||
        fs->va.cur_ef.hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        uint32_t rcrd_cnt;
        if (swicc_disk_file_rcrd_cnt(fs->va.cur_tree, &fs->va.cur_ef,
                                     &rcrd_cnt) == SWICC_RET_SUCCESS)
        {
            swicc_fs_rcrd_idx_kt idx_abs = idx;
            /* Cyclic EFs have records indexed relative current record. */
            if (fs->va.cur_ef.hdr_item.type ==
                SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
            {
                idx_abs =
                    (swicc_fs_rcrd_idx_kt)((uint32_t)(fs->va.cur_rcrd.idx +
                                                      idx) %
                                           rcrd_cnt);
            }
            swicc_fs_rcrd_st const rcrd = {.idx = idx_abs};
            fs->va.cur_rcrd = rcrd;
            return SWICC_RET_SUCCESS;
        }
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_va_select_data_offset(swicc_fs_st *const fs,
                                         uint32_t offset_prel)
{
    return SWICC_RET_UNKNOWN;
}

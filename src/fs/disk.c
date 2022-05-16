#include "uicc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Used when creating LUTs. The 'start' count determines that size of the
 * smallest created LUT (measured in entry counts). If the 'start' count is too
 * small, the LUT will be resized by the 'resize' amount.
 */
#define LUT_COUNT_START 64U
#define LUT_COUNT_RESIZE 8U

/**
 * @brief Insert an entry into a LUT (resizes the LUT if needed).
 * @param lut
 * @param entry_item1 This will be placed in buffer 1 and must have size equal
 * to the item size 1.
 * @param entry_item2 This will be placed in buffer 2 and must have size equal
 * to the item size 2.
 * @return Return code.
 */
static uicc_ret_et lut_insert(uicc_disk_lut_st *const lut,
                              uint8_t const *const entry_item1,
                              uint8_t const *const entry_item2)
{
    /* Check if need to resize the LUT to fit more items. */
    if (lut->count >= lut->count_max)
    {
        lut->count_max += LUT_COUNT_RESIZE;
        uint8_t *const buf1_new = malloc(lut->count_max * lut->size_item1);
        uint8_t *const buf2_new = malloc(lut->count_max * lut->size_item2);
        if (buf1_new == NULL || buf2_new == NULL)
        {
            free(buf1_new);
            free(buf2_new);
            return UICC_RET_ERROR;
        }
        lut->buf1 = buf1_new;
        lut->buf2 = buf2_new;
    }

    /**
     * Insert such that item 1 of all entries are arranged in increasing order.
     */
    uint32_t start = 0U;
    uint32_t end = lut->count;
    uint32_t mid;
    while (start < end)
    {
        mid = (start + end) / 2U;
        /* value_mid < value_new */
        if (memcmp(&lut->buf1[lut->size_item1 * mid], entry_item1,
                   lut->size_item1) < 0)
        {
            start = mid + 1U;
        }
        else
        {
            end = mid;
        }
    }
    memmove(&lut->buf1[lut->size_item1 * (start + 1U)],
            &lut->buf1[lut->size_item1 * (start)],
            lut->size_item1 * (lut->count - start));
    memmove(&lut->buf2[lut->size_item2 * (start + 1U)],
            &lut->buf2[lut->size_item2 * (start)],
            lut->size_item2 * (lut->count - start));
    memcpy(&lut->buf1[lut->size_item1 * start], entry_item1, lut->size_item1);
    memcpy(&lut->buf2[lut->size_item2 * start], entry_item2, lut->size_item2);
    lut->count += 1U;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_disk_tree_file_foreach(uicc_disk_tree_st *const tree,
                                        fs_file_foreach_cb *const cb,
                                        void *const userdata)
{
    uicc_ret_et ret = UICC_RET_ERROR;

    uicc_fs_file_hdr_st file_hdr;
    uicc_fs_item_hdr_raw_st const *const item_hdr_raw =
        (uicc_fs_item_hdr_raw_st *)tree->buf;
    uicc_fs_file_hdr_raw_st const *const file_hdr_raw =
        (uicc_fs_file_hdr_raw_st *)item_hdr_raw;
    ret = uicc_fs_item_hdr_prs(item_hdr_raw, &file_hdr.item);
    if (ret == UICC_RET_SUCCESS)
    {
        ret = uicc_fs_file_hdr_prs(file_hdr_raw, &file_hdr);
        if (ret != UICC_RET_SUCCESS)
        {
            return ret;
        }
        file_hdr.item.offset_trel = 0U;
        /* Perform the per-file operation also for the tree. */
        ret = cb(tree, &file_hdr, userdata);
        if (ret != UICC_RET_SUCCESS)
        {
            return ret;
        }

        if (file_hdr.item.type == UICC_FS_ITEM_TYPE_FILE_MF ||
            file_hdr.item.type == UICC_FS_ITEM_TYPE_FILE_ADF)
        {
            uint32_t stack_data_idx[UICC_FS_DEPTH_MAX] = {
                file_hdr.item.type == UICC_FS_ITEM_TYPE_FILE_ADF
                    ? sizeof(uicc_fs_adf_hdr_raw_st)
                    : sizeof(uicc_fs_file_hdr_raw_st),
                0};
            uint32_t depth = 1U; /* Inside the tree so 1 already. */
            while (depth < UICC_FS_DEPTH_MAX)
            {
                if (stack_data_idx[depth - 1U] >= file_hdr.item.size)
                {
                    depth -= 1U;
                    if (depth < 1U)
                    {
                        /* Not an error, just means we are done. */
                        ret = UICC_RET_SUCCESS;
                        break;
                    }
                    /* Restore old data idx. */
                    stack_data_idx[depth - 1U] = stack_data_idx[depth];
                }

                uicc_fs_file_hdr_st file_nested_hdr;
                uicc_fs_item_hdr_raw_st const *const item_nested_hdr_raw =
                    (uicc_fs_item_hdr_raw_st *)&tree
                        ->buf[stack_data_idx[depth - 1U]];
                ret = uicc_fs_item_hdr_prs(item_nested_hdr_raw,
                                           &file_nested_hdr.item);
                if (ret != UICC_RET_SUCCESS)
                {
                    break;
                }
                uicc_fs_file_hdr_raw_st const *const file_nested_hdr_raw =
                    (uicc_fs_file_hdr_raw_st *)item_nested_hdr_raw;
                ret =
                    uicc_fs_file_hdr_prs(file_nested_hdr_raw, &file_nested_hdr);
                if (ret != UICC_RET_SUCCESS)
                {
                    break;
                }
                file_nested_hdr.item.offset_trel = stack_data_idx[depth - 1U];

                /* Perform the per-file operation. */
                ret = cb(tree, &file_nested_hdr, userdata);
                if (ret != UICC_RET_SUCCESS)
                {
                    break;
                }

                switch (file_nested_hdr.item.type)
                {
                case UICC_FS_ITEM_TYPE_FILE_MF:
                case UICC_FS_ITEM_TYPE_FILE_ADF:
                case UICC_FS_ITEM_TYPE_FILE_DF:
                    stack_data_idx[depth] = stack_data_idx[depth - 1U];
                    depth += 1U;
                    uint64_t const data_idx_new =
                        stack_data_idx[depth - 1U] +
                        (file_nested_hdr.item.type == UICC_FS_ITEM_TYPE_FILE_ADF
                             ? sizeof(uicc_fs_adf_hdr_raw_st)
                             : sizeof(*file_nested_hdr_raw));
                    if (data_idx_new > UINT32_MAX)
                    {
                        /* Data index would overflow. */
                        uicc_disk_lutsid_empty(tree);
                        ret = UICC_RET_ERROR;
                        break;
                    }
                    /* Safe cast due to overflow check. */
                    stack_data_idx[depth - 1U] = (uint32_t)(data_idx_new);
                    break;
                case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
                case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
                case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
                    stack_data_idx[depth - 1U] += file_nested_hdr.item.size;
                    break;
                case UICC_FS_ITEM_TYPE_INVALID:
                    ret = UICC_RET_ERROR;
                    break;
                default:
                    break;
                }

                if (ret != UICC_RET_SUCCESS)
                {
                    break;
                }
            }
        }
        else
        {
            /* Only MFs and ADFs can be roots of trees. */
            ret = UICC_RET_ERROR;
        }
    }
    return ret;
}

uicc_ret_et uicc_disk_tree_iter(uicc_disk_st *const disk,
                                uicc_disk_tree_iter_st *const tree_iter)
{
    if (disk->root != NULL)
    {
        tree_iter->tree = disk->root;
        tree_iter->tree_idx = 0U;
        return UICC_RET_SUCCESS;
    }
    return UICC_RET_ERROR;
}

uicc_ret_et uicc_disk_tree_iter_next(uicc_disk_tree_iter_st *const tree_iter,
                                     uicc_disk_tree_st **const tree)
{
    if (tree_iter->tree->next != NULL)
    {
        tree_iter->tree = tree_iter->tree->next;
        /* Unsafe cast that relies on there being fewer than 256 trees. */
        tree_iter->tree_idx = (uint8_t)(tree_iter->tree_idx + 1U);
        *tree = tree_iter->tree;
        return UICC_RET_SUCCESS;
    }
    return UICC_RET_ERROR;
}

uicc_ret_et uicc_disk_tree_iter_idx(uicc_disk_tree_iter_st *const tree_iter,
                                    uint8_t const tree_idx,
                                    uicc_disk_tree_st **const tree)
{
    while (tree_iter->tree_idx != tree_idx)
    {
        uicc_ret_et ret_iter = uicc_disk_tree_iter_next(tree_iter, tree);
        if (ret_iter != UICC_RET_SUCCESS)
        {
            return ret_iter;
        }
    }
    if (tree_iter->tree_idx == tree_idx)
    {
        *tree = tree_iter->tree;
    }
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_disk_load(uicc_disk_st *const disk,
                           char const *const disk_path)
{
    uicc_ret_et ret = UICC_RET_ERROR;
    if (disk->root != NULL)
    {
        /* Get rid of the current disk first before loading a new one. */
        return ret;
    }

    /* Clear disk so that all the members have a known initial state. */
    memset(disk, 0U, sizeof(*disk));

    FILE *f = fopen(disk_path, "rb");
    if (!(f == NULL))
    {
        if (fseek(f, 0, SEEK_END) == 0)
        {
            int64_t f_len = ftell(f);
            if (f_len >= 0 && f_len <= UINT32_MAX)
            {
                if (fseek(f, 0, SEEK_SET) == 0)
                {
                    uint8_t magic_expected[UICC_DISK_MAGIC_LEN] =
                        UICC_DISK_MAGIC;
                    uint8_t magic[UICC_DISK_MAGIC_LEN];
                    if (fread(&magic, UICC_DISK_MAGIC_LEN, 1U, f) == 1U)
                    {
                        if (memcmp(magic, magic_expected,
                                   UICC_DISK_MAGIC_LEN) == 0)
                        {
                            /**
                             * Parse the root (forest of trees) contained in the
                             * file.
                             */
                            uicc_disk_tree_st *tree = NULL;
                            uicc_ret_et ret_item = UICC_RET_SUCCESS;
                            uint32_t data_idx = UICC_DISK_MAGIC_LEN;
                            uint8_t tree_idx = 0U;
                            /**
                             * Assume the file length matches the disk length
                             * (no extra bytes).
                             */
                            while (data_idx < f_len)
                            {
                                /* Check if creating the first tree. */
                                if (tree == NULL)
                                {
                                    tree = malloc(sizeof(*tree));
                                    if (tree == NULL)
                                    {
                                        ret_item = UICC_RET_ERROR;
                                        break;
                                    }
                                    disk->root = tree;
                                }
                                else
                                {
                                    tree->next = malloc(sizeof(*tree));
                                    if (tree->next == NULL)
                                    {
                                        ret_item = UICC_RET_ERROR;
                                        break;
                                    }
                                    tree = tree->next;
                                }
                                memset(tree, 0U, sizeof(*tree));

                                uicc_fs_item_hdr_raw_st item_hdr_raw;
                                uicc_fs_item_hdr_st item_hdr;
                                if (fread(&item_hdr_raw, sizeof(item_hdr_raw),
                                          1U, f) != 1U)
                                {
                                    ret_item = UICC_RET_ERROR;
                                    break;
                                }

                                /* Got a header. */
                                ret_item = uicc_fs_item_hdr_prs(&item_hdr_raw,
                                                                &item_hdr);
                                if (ret_item != UICC_RET_SUCCESS)
                                {
                                    break;
                                }
                                /**
                                 * Make sure all trees are valid and the first
                                 * one is the MF.
                                 */
                                if (item_hdr.type ==
                                        UICC_FS_ITEM_TYPE_INVALID ||
                                    (tree_idx == 0 &&
                                     item_hdr.type !=
                                         UICC_FS_ITEM_TYPE_FILE_MF))
                                {
                                    ret_item = UICC_RET_ERROR;
                                    break;
                                }

                                /**
                                 * Know the size of the item so can allocate the
                                 * exact amount of space needed.
                                 */
                                tree->buf = malloc(item_hdr.size);
                                if (tree->buf == NULL)
                                {
                                    ret_item = UICC_RET_ERROR;
                                    break;
                                }
                                tree->size = item_hdr.size;

                                /**
                                 * Read the rest of the item first before
                                 * writing the header to save on a memcpy if
                                 * this read fails.
                                 */
                                uint64_t fred_ret =
                                    fread(&tree->buf[sizeof(item_hdr_raw)],
                                          item_hdr.size - sizeof(item_hdr_raw),
                                          1U, f);
                                if (fred_ret != 1U)
                                {
                                    ret_item = UICC_RET_ERROR;
                                    break;
                                }
                                /* Copy the header. */
                                memcpy(tree->buf, &item_hdr_raw,
                                       sizeof(item_hdr_raw));
                                tree->len = item_hdr.size;

                                /* Keep track how much of the file was read. */
                                data_idx += tree->len;

                                /* Keep track of the number of trees. */
                                /**
                                 * Unsafe cast that relies on there being fewer
                                 * than 256 trees.
                                 */
                                tree_idx = (uint8_t)(tree_idx + 1U);
                            }
                            /**
                             * By default the return code is 'success' so if
                             * something in the while loop failed, it will be
                             * modified to reflect that.
                             */
                            ret = ret_item;
                        }
                    }
                }
            }
        }
        if (fclose(f) != 0)
        {
            ret = UICC_RET_ERROR;
        }
    }
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_disk_root_empty(disk);
    }
    else
    {
        /* Create all the LUTs. */
        uicc_disk_tree_st *tree = disk->root;
        if (tree == NULL)
        {
            ret = UICC_RET_ERROR;
        }
        while (tree != NULL)
        {
            ret = uicc_disk_lutsid_rebuild(disk, tree);
            if (ret != UICC_RET_SUCCESS)
            {
                break;
            }
            tree = tree->next;
        }
        if (ret == UICC_RET_SUCCESS)
        {
            ret = uicc_disk_lutid_rebuild(disk);
        }
    }
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_disk_root_empty(disk);
    }
    return ret;
}

void uicc_disk_unload(uicc_disk_st *const disk)
{
    /* This also frees the SID LUT of all trees. */
    uicc_disk_root_empty(disk);

    uicc_disk_lutid_empty(disk);
    memset(disk, 0U, sizeof(*disk));
}

void uicc_disk_root_empty(uicc_disk_st *const disk)
{
    uicc_disk_tree_st *tree = disk->root;
    while (tree != NULL)
    {
        if (tree->buf != NULL)
        {
            free(tree->buf);
        }

        /* Free the SID LUT of this tree. */
        uicc_disk_lutsid_empty(tree);

        uicc_disk_tree_st *const tree_next = tree->next;
        free(tree);
        tree = tree_next;
    }
    disk->root = NULL;
}

void uicc_disk_lutsid_empty(uicc_disk_tree_st *const tree)
{
    uicc_disk_lut_st *lutsid = &tree->lutsid;
    if (lutsid->buf1 != NULL)
    {
        free(lutsid->buf1);
    }
    if (lutsid->buf2 != NULL)
    {
        free(lutsid->buf2);
    }
    memset(&tree->lutsid, 0U, sizeof(tree->lutsid));
}

void uicc_disk_lutid_empty(uicc_disk_st *const disk)
{
    uicc_disk_lut_st *lutid = &disk->lutid;
    free(lutid->buf1);
    free(lutid->buf2);
    memset(&disk->lutid, 0U, sizeof(disk->lutid));
}

uicc_ret_et uicc_disk_lutsid_lookup(uicc_disk_tree_st *const tree,
                                    uicc_fs_sid_kt const sid,
                                    uicc_fs_file_hdr_st *const file)
{
    uicc_disk_lut_st *const lutsid = &tree->lutsid;

    /* Make sure the SID LUT is as expected. */
    if (lutsid->buf1 == NULL || lutsid->size_item1 != sizeof(uicc_fs_sid_kt) ||
        lutsid->size_item2 != sizeof(uint32_t))
    {
        return UICC_RET_ERROR;
    }

    /* Find the file by SID. */
    uint32_t entry_idx = 0U;
    while (entry_idx < lutsid->count)
    {
        if (memcmp(&lutsid->buf1[lutsid->size_item1 * entry_idx], &sid,
                   lutsid->size_item1) == 0)
        {
            break;
        }
        entry_idx += 1U;
    }
    if (entry_idx >= lutsid->count)
    {
        return UICC_RET_FS_NOT_FOUND;
    }

    uint32_t const offset =
        *(uint32_t *)&lutsid->buf2[lutsid->size_item2 * entry_idx];
    /* Offset too large. */
    if (offset >= tree->len)
    {
        return UICC_RET_ERROR;
    }

    uicc_fs_file_hdr_raw_st *const file_raw =
        (uicc_fs_file_hdr_raw_st *)&tree->buf[offset];
    if (uicc_fs_item_hdr_prs(&file_raw->item, &file->item) != UICC_RET_SUCCESS)
    {
        return UICC_RET_ERROR;
    }
    file->item.offset_trel = offset;
    if (uicc_fs_file_hdr_prs(file_raw, file) != UICC_RET_SUCCESS)
    {
        return UICC_RET_ERROR;
    }

    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_disk_lutid_lookup(uicc_disk_st *const disk,
                                   uicc_disk_tree_st **const tree,
                                   uicc_fs_id_kt const id,
                                   uicc_fs_file_hdr_st *const file)
{
    uicc_disk_lut_st *const lutid = &disk->lutid;

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
        if (memcmp(&lutid->buf1[lutid->size_item1 * entry_idx], &id,
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

    uint32_t const offset =
        *(uint32_t *)&lutid->buf2[(lutid->size_item2 * entry_idx) + 0U];
    uint8_t const tree_idx =
        lutid->buf2[(lutid->size_item2 * entry_idx) + sizeof(uint32_t)];

    /* Find the tree in which the file resides. */
    uicc_disk_tree_iter_st tree_iter;
    if (uicc_disk_tree_iter(disk, &tree_iter) != UICC_RET_SUCCESS ||
        uicc_disk_tree_iter_idx(&tree_iter, tree_idx, tree) !=
            UICC_RET_SUCCESS ||
        (*tree)->len <= offset)
    {
        return UICC_RET_ERROR;
    }

    /* Offset too large. */
    if (offset >= (*tree)->len)
    {
        return UICC_RET_ERROR;
    }
    uicc_fs_file_hdr_raw_st *const file_raw =
        (uicc_fs_file_hdr_raw_st *)&(*tree)->buf[offset];
    if (uicc_fs_item_hdr_prs(&file_raw->item, &file->item) != UICC_RET_SUCCESS)
    {
        return UICC_RET_ERROR;
    }
    file->item.offset_trel = offset;
    if (uicc_fs_file_hdr_prs(file_raw, file) != UICC_RET_SUCCESS)
    {
        return UICC_RET_ERROR;
    }

    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_disk_save(uicc_disk_st *const disk,
                           char const *const disk_path)
{
    uicc_ret_et ret = UICC_RET_ERROR;
    FILE *f = fopen(disk_path, "wb");
    if (f != NULL)
    {
        uint8_t magic[UICC_DISK_MAGIC_LEN] = UICC_DISK_MAGIC;
        if (fwrite(magic, UICC_DISK_MAGIC_LEN, 1U, f) == 1U)
        {
            uicc_disk_tree_st *tree = disk->root;
            while (tree != NULL)
            {
                if (fwrite(tree->buf, tree->len, 1U, f) != 1U)
                {
                    ret = UICC_RET_ERROR;
                    break;
                }
                tree = tree->next;
                ret = UICC_RET_SUCCESS;
            }
        }
        if (fclose(f) != 0)
        {
            ret = UICC_RET_ERROR;
        }
    }
    return ret;
}

static fs_file_foreach_cb lutid_rebuild_cb;
typedef struct lut_id_rebuild_cb_userdata_s
{
    uicc_disk_lut_st *lut;
    uint8_t tree_idx;
} lut_id_rebuild_cb_userdata_st;
static uicc_ret_et lutid_rebuild_cb(uicc_disk_tree_st *const tree,
                                    uicc_fs_file_hdr_st *const file,
                                    void *const userdata)
{
    if (file->id == UICC_FS_ID_MISSING)
    {
        return UICC_RET_SUCCESS;
    }

    /* Insert the ID + offset into the ID LUT. */
    lut_id_rebuild_cb_userdata_st const *const userdata_struct = userdata;
    uicc_disk_lut_st *const lutid = userdata_struct->lut;
    uint8_t entry_item1[] = {
        /* Safe cast, just extracting the upper byte. */
        (uint8_t)((file->id & 0xFF00) >> 8U),
        /* Safe cast, just extracting the lower byte. */
        (uint8_t)(file->id & 0x00FF),
    };
    uint8_t entry_item2[lutid->size_item2];
    memcpy(&entry_item2[0U], &file->item.offset_trel, sizeof(uint32_t));
    memcpy(&entry_item2[sizeof(uint32_t)], &userdata_struct->tree_idx,
           sizeof(uint8_t));
    return lut_insert(lutid, entry_item1, entry_item2);
}

uicc_ret_et uicc_disk_lutid_rebuild(uicc_disk_st *const disk)
{
    uicc_ret_et ret = UICC_RET_ERROR;

    /* Cleanup the old ID LUT before rebuilding it. */
    uicc_disk_lutid_empty(disk);
    disk->lutid.size_item1 = sizeof(uicc_fs_id_kt);
    disk->lutid.size_item2 =
        sizeof(uint32_t) + sizeof(uint8_t); /* Offset + tree index */
    disk->lutid.count_max = LUT_COUNT_START;
    disk->lutid.count = 0U;
    disk->lutid.buf1 = malloc(disk->lutid.count_max * disk->lutid.size_item1);
    disk->lutid.buf2 = malloc(disk->lutid.count_max * disk->lutid.size_item2);
    if (disk->lutid.buf1 == NULL || disk->lutid.buf2 == NULL)
    {
        uicc_disk_lutid_empty(disk);
        return UICC_RET_ERROR;
    }

    uicc_disk_tree_st *tree = disk->root;
    uint8_t tree_idx = 0U;
    while (tree != NULL)
    {
        lut_id_rebuild_cb_userdata_st userdata = {.lut = &disk->lutid,
                                                  .tree_idx = tree_idx};
        ret = uicc_disk_tree_file_foreach(tree, lutid_rebuild_cb, &userdata);
        if (ret != UICC_RET_SUCCESS)
        {
            uicc_disk_lutid_empty(disk);
            break;
        }

        tree = tree->next;

        /**
         * Unsafe cast that relies on there being fewer than 256 trees in the
         * forest.
         */
        tree_idx = (uint8_t)(tree_idx + 1U);
    }
    return ret;
}

static fs_file_foreach_cb lutsid_rebuild_cb;
static uicc_ret_et lutsid_rebuild_cb(uicc_disk_tree_st *const tree,
                                     uicc_fs_file_hdr_st *const file,
                                     void *const userdata)
{
    if (file->sid == UICC_FS_SID_MISSING)
    {
        return UICC_RET_SUCCESS;
    }

    /* Insert the SID + offset into the SID LUT. */
    return lut_insert(&tree->lutsid, (uint8_t *)&file->sid,
                      (uint8_t *)&file->item.offset_trel);
}

uicc_ret_et uicc_disk_lutsid_rebuild(uicc_disk_st *const disk,
                                     uicc_disk_tree_st *const tree)
{
    /* Cleanup the old SID LUT before rebuilding it. */
    uicc_disk_lutsid_empty(tree);
    tree->lutsid.size_item1 = sizeof(uicc_fs_sid_kt);
    tree->lutsid.size_item2 = sizeof(uint32_t);
    tree->lutsid.count_max = LUT_COUNT_START;
    tree->lutsid.count = 0U;
    tree->lutsid.buf1 =
        malloc(tree->lutsid.count_max * tree->lutsid.size_item1);
    tree->lutsid.buf2 =
        malloc(tree->lutsid.count_max * tree->lutsid.size_item2);
    if (tree->lutsid.buf1 == NULL || tree->lutsid.buf2 == NULL)
    {
        uicc_disk_lutsid_empty(tree);
        return UICC_RET_ERROR;
    }

    uicc_ret_et ret =
        uicc_disk_tree_file_foreach(tree, lutsid_rebuild_cb, NULL);
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_disk_lutsid_empty(tree);
    }
    return ret;
}

uicc_ret_et uicc_disk_file_rcrd(uicc_disk_tree_st *const tree,
                                uicc_fs_file_hdr_st *const file,
                                uicc_fs_rcrd_idx_kt const idx,
                                uint8_t **const buf, uint8_t *const len)
{
    /* Only linear-fixed or cyclic files have records. */
    if (file->item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||
        file->item.type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        /* Indexes begin at the first record in the array. */
        uicc_fs_ef_linearfixed_hdr_raw_st *ef_linearfixed =
            (uicc_fs_ef_linearfixed_hdr_raw_st *)&tree
                ->buf[file->item.offset_trel];
        uicc_fs_ef_cyclic_hdr_raw_st *ef_cyclic =
            (uicc_fs_ef_linearfixed_hdr_raw_st *)&tree
                ->buf[file->item.offset_trel];

        uint8_t const rcrd_size =
            file->item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED
                ? ef_linearfixed->rcrd_size
                : ef_cyclic->rcrd_size;
        uint32_t const hdr_size =
            file->item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED
                ? sizeof(*ef_linearfixed)
                : sizeof(*ef_cyclic);

        uint32_t rcrd_cnt;
        uicc_ret_et const ret_rcrd_cnt =
            uicc_disk_file_rcrd_cnt(tree, file, &rcrd_cnt);
        if (ret_rcrd_cnt == UICC_RET_SUCCESS)
        {
            if (idx >= ret_rcrd_cnt)
            {
                return UICC_RET_FS_NOT_FOUND;
            }
            /* Safe cast, this sum will never be greater than uint64 max. */
            uint64_t const rcrd_offset =
                (uint64_t)(file->item.offset_trel + hdr_size) +
                (uint64_t)(rcrd_size * idx);
            if (rcrd_offset > UINT32_MAX)
            {
                return UICC_RET_ERROR;
            }
            /* Safe cast due to check on size. */
            *buf = &tree->buf[(uint32_t)rcrd_offset];
            *len = rcrd_size;
            return UICC_RET_SUCCESS;
        }
    }
    return UICC_RET_ERROR;
}

uicc_ret_et uicc_disk_file_rcrd_cnt(uicc_disk_tree_st *const tree,
                                    uicc_fs_file_hdr_st *const file,
                                    uint32_t *const rcrd_cnt)
{
    if (file->item.type != UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED)
    {
        /* Indexes begin at the first record in the array. */
        uicc_fs_ef_linearfixed_hdr_raw_st *ef =
            (uicc_fs_ef_linearfixed_hdr_raw_st *)&tree
                ->buf[file->item.offset_trel];
        /* Safe case since there can only be UINT32_MAX/1 records at most. */
        *rcrd_cnt =
            (uint32_t)((ef->file.item.size - sizeof(*ef)) / ef->rcrd_size);
        return UICC_RET_SUCCESS;
    }
    else if (file->item.type != UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        /* Indexes begin at the last/selected record in the array. */
        uicc_fs_ef_cyclic_hdr_raw_st *ef =
            (uicc_fs_ef_cyclic_hdr_raw_st *)&tree->buf[file->item.offset_trel];
        /* Safe case since there can only be UINT32_MAX/1 records at most. */
        *rcrd_cnt =
            (uint32_t)((ef->file.item.size - sizeof(*ef)) / ef->rcrd_size);
        return UICC_RET_SUCCESS;
    }
    return UICC_RET_ERROR;
}

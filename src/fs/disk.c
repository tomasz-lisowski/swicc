#include "swicc/fs/common.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <swicc/swicc.h>

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
static swicc_ret_et lut_insert(swicc_disk_lut_st *const lut,
                               uint8_t const *const entry_item1,
                               uint8_t const *const entry_item2)
{
    /* Check if need to resize the LUT to fit more items. */
    if (lut->count >= lut->count_max)
    {
        lut->count_max += LUT_COUNT_RESIZE;
        uint8_t *const buf1_new =
            realloc(lut->buf1, lut->count_max * lut->size_item1);
        uint8_t *const buf2_new =
            realloc(lut->buf2, lut->count_max * lut->size_item2);
        if (buf1_new == NULL || buf2_new == NULL)
        {
            free(buf1_new);
            free(buf2_new);
            return SWICC_RET_ERROR;
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
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_disk_load(swicc_disk_st *const disk,
                             char const *const disk_path)
{
    if (disk == NULL || disk_path == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
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
                    uint8_t magic_expected[SWICC_DISK_MAGIC_LEN] =
                        SWICC_DISK_MAGIC;
                    uint8_t magic[SWICC_DISK_MAGIC_LEN];
                    if (fread(&magic, SWICC_DISK_MAGIC_LEN, 1U, f) == 1U)
                    {
                        if (memcmp(magic, magic_expected,
                                   SWICC_DISK_MAGIC_LEN) == 0)
                        {
                            /**
                             * Parse the root (forest of trees) contained in the
                             * file.
                             */
                            swicc_disk_tree_st *tree = NULL;
                            swicc_ret_et ret_item = SWICC_RET_SUCCESS;
                            uint32_t data_idx = SWICC_DISK_MAGIC_LEN;
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
                                        ret_item = SWICC_RET_ERROR;
                                        break;
                                    }
                                    disk->root = tree;
                                }
                                else
                                {
                                    tree->next = malloc(sizeof(*tree));
                                    if (tree->next == NULL)
                                    {
                                        ret_item = SWICC_RET_ERROR;
                                        break;
                                    }
                                    tree = tree->next;
                                }
                                memset(tree, 0U, sizeof(*tree));

                                /* Got a header. */
                                swicc_fs_item_hdr_raw_st item_hdr_raw;
                                swicc_fs_item_hdr_st item_hdr;
                                if (fread(&item_hdr_raw, sizeof(item_hdr_raw),
                                          1U, f) != 1U)
                                {
                                    ret_item = SWICC_RET_ERROR;
                                    break;
                                }
                                swicc_fs_item_hdr_prs(&item_hdr_raw, 0U,
                                                      &item_hdr);
                                /**
                                 * Make sure all trees are valid, the first
                                 * one is the MF, and all other ones are ADFs.
                                 */
                                if (item_hdr.type ==
                                        SWICC_FS_ITEM_TYPE_INVALID ||
                                    (tree_idx == 0 &&
                                     item_hdr.type !=
                                         SWICC_FS_ITEM_TYPE_FILE_MF) ||
                                    (tree_idx != 0 &&
                                     item_hdr.type !=
                                         SWICC_FS_ITEM_TYPE_FILE_ADF))
                                {
                                    ret_item = SWICC_RET_ERROR;
                                    break;
                                }

                                /**
                                 * Know the size of the item so can allocate the
                                 * exact amount of space needed.
                                 */
                                tree->buf = malloc(item_hdr.size);
                                if (tree->buf == NULL)
                                {
                                    ret_item = SWICC_RET_ERROR;
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
                                    ret_item = SWICC_RET_ERROR;
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
            ret = SWICC_RET_ERROR;
        }
    }
    if (ret != SWICC_RET_SUCCESS)
    {
        swicc_disk_root_empty(disk);
    }
    else
    {
        /* Create all the LUTs. */
        swicc_disk_tree_st *tree = disk->root;
        if (tree == NULL)
        {
            ret = SWICC_RET_ERROR;
        }
        while (tree != NULL)
        {
            ret = swicc_disk_lutsid_rebuild(disk, tree);
            if (ret != SWICC_RET_SUCCESS)
            {
                break;
            }
            tree = tree->next;
        }
        if (ret == SWICC_RET_SUCCESS)
        {
            ret = swicc_disk_lutid_rebuild(disk);
        }
    }
    if (ret != SWICC_RET_SUCCESS)
    {
        swicc_disk_root_empty(disk);
    }
    return ret;
}

void swicc_disk_unload(swicc_disk_st *const disk)
{
    if (disk == NULL)
    {
        /**
         * It's worth it to keep this function return void even if this return
         * would ideally indicate 'bad parameters'.
         */
        return;
    }

    /* This also frees the SID LUT of all trees. */
    swicc_disk_root_empty(disk);

    swicc_disk_lutid_empty(disk);
    memset(disk, 0U, sizeof(*disk));
}

swicc_ret_et swicc_disk_save(swicc_disk_st const *const disk,
                             char const *const disk_path)
{
    if (disk == NULL || disk_path == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    FILE *f = fopen(disk_path, "wb");
    if (f != NULL)
    {
        uint8_t magic[SWICC_DISK_MAGIC_LEN] = SWICC_DISK_MAGIC;
        if (fwrite(magic, SWICC_DISK_MAGIC_LEN, 1U, f) == 1U)
        {
            swicc_disk_tree_st *tree = disk->root;
            while (tree != NULL)
            {
                if (fwrite(tree->buf, tree->len, 1U, f) != 1U)
                {
                    ret = SWICC_RET_ERROR;
                    break;
                }
                tree = tree->next;
                ret = SWICC_RET_SUCCESS;
            }
        }
        if (fclose(f) != 0)
        {
            ret = SWICC_RET_ERROR;
        }
    }
    return ret;
}

swicc_ret_et swicc_disk_file_foreach(swicc_disk_tree_st *const tree,
                                     swicc_fs_file_st *const file,
                                     fs_file_foreach_cb *const cb,
                                     void *const userdata, bool const recurse)
{
    /* User-data can be null, other params not. */
    if (tree == NULL || file == NULL || cb == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;

    swicc_fs_file_st *const file_root = file;

    /* Perform the per-file operation also for the tree. */
    ret = cb(tree, file_root, userdata);
    if (ret != SWICC_RET_SUCCESS)
    {
        return ret;
    }

    if (file_root->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_MF ||
        file_root->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_ADF)
    {
        uint32_t const root_hdr_len =
            swicc_fs_item_hdr_raw_size[file_root->hdr_item.type];
        uint32_t stack_data_idx[SWICC_FS_DEPTH_MAX] = {root_hdr_len, 0U, 0U};
        uint32_t depth = 1U; /* Inside the tree so 1 already. */
        while (depth < SWICC_FS_DEPTH_MAX)
        {
            if (stack_data_idx[depth - 1U] >= file_root->hdr_item.size)
            {
                depth -= 1U;
                if (depth < 1U ||
                    stack_data_idx[depth] == file_root->hdr_item.size)
                {
                    /* Not an error, just means we are done. */
                    ret = SWICC_RET_SUCCESS;
                    break;
                }
                /* Restore old data idx. */
                stack_data_idx[depth - 1U] = stack_data_idx[depth];
            }

            swicc_fs_file_st file_nstd;
            ret =
                swicc_fs_file_prs(tree, stack_data_idx[depth - 1U], &file_nstd);
            if (ret != SWICC_RET_SUCCESS)
            {
                break;
            }
            uint32_t const nstd_hdr_len =
                swicc_fs_item_hdr_raw_size[file_nstd.hdr_item.type];

            /* Perform the per-file operation. */
            ret = cb(tree, &file_nstd, userdata);
            if (ret != SWICC_RET_SUCCESS)
            {
                break;
            }

            switch (file_nstd.hdr_item.type)
            {
            case SWICC_FS_ITEM_TYPE_FILE_MF:
            case SWICC_FS_ITEM_TYPE_FILE_ADF:
            case SWICC_FS_ITEM_TYPE_FILE_DF:
                if (recurse)
                {
                    stack_data_idx[depth] = stack_data_idx[depth - 1U];
                    depth += 1U;
                    uint64_t const data_idx_new =
                        stack_data_idx[depth - 1U] + nstd_hdr_len;
                    if (data_idx_new > UINT32_MAX)
                    {
                        /* Data index would overflow. */
                        ret = SWICC_RET_ERROR;
                        break;
                    }
                    /* Safe cast due to overflow check. */
                    stack_data_idx[depth - 1U] = (uint32_t)(data_idx_new);
                    break;
                }
                else
                {
                    __attribute__((fallthrough));
                }
            case SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
            case SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
            case SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
                stack_data_idx[depth - 1U] += file_nstd.hdr_item.size;
                break;
            case SWICC_FS_ITEM_TYPE_INVALID:
                ret = SWICC_RET_ERROR;
                break;
            default:
                break;
            }

            if (ret != SWICC_RET_SUCCESS)
            {
                break;
            }
        }
    }
    else
    {
        /* Only MFs and ADFs can be roots of trees. */
        ret = SWICC_RET_ERROR;
    }
    return ret;
}

swicc_ret_et swicc_disk_tree_iter(swicc_disk_st const *const disk,
                                  swicc_disk_tree_iter_st *const tree_iter)
{
    if (disk == NULL || tree_iter == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }
    if (disk->root != NULL)
    {
        tree_iter->tree = disk->root;
        tree_iter->tree_idx = 0U;
        return SWICC_RET_SUCCESS;
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_disk_tree_iter_next(swicc_disk_tree_iter_st *const tree_iter,
                                       swicc_disk_tree_st **const tree)
{
    if (tree_iter == NULL || tree == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }
    if (tree_iter->tree->next != NULL)
    {
        tree_iter->tree = tree_iter->tree->next;
        /* Unsafe cast that relies on there being fewer than 256 trees. */
        tree_iter->tree_idx = (uint8_t)(tree_iter->tree_idx + 1U);
        *tree = tree_iter->tree;
        return SWICC_RET_SUCCESS;
    }
    else
    {
        /* Did not find tree. */
        return SWICC_RET_FS_NOT_FOUND;
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_disk_tree_iter_idx(swicc_disk_tree_iter_st *const tree_iter,
                                      uint8_t const tree_idx,
                                      swicc_disk_tree_st **const tree)
{
    if (tree_iter == NULL || tree == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }
    while (tree_iter->tree_idx != tree_idx)
    {
        swicc_ret_et ret_iter = swicc_disk_tree_iter_next(tree_iter, tree);
        if (ret_iter != SWICC_RET_SUCCESS)
        {
            return ret_iter;
        }
    }
    if (tree_iter->tree_idx == tree_idx)
    {
        *tree = tree_iter->tree;
    }
    return SWICC_RET_SUCCESS;
}

void swicc_disk_root_empty(swicc_disk_st *const disk)
{
    if (disk == NULL)
    {
        return;
    }
    swicc_disk_tree_st *tree = disk->root;
    while (tree != NULL)
    {
        if (tree->buf != NULL)
        {
            free(tree->buf);
        }

        /* Free the SID LUT of this tree. */
        swicc_disk_lutsid_empty(tree);

        swicc_disk_tree_st *const tree_next = tree->next;
        free(tree);
        tree = tree_next;
    }
    disk->root = NULL;
    /* Since there will be no trees left, the ID LUT shall also be destroyed. */
    swicc_disk_lutid_empty(disk);
}

void swicc_disk_lutsid_empty(swicc_disk_tree_st *const tree)
{
    if (tree == NULL)
    {
        return;
    }
    swicc_disk_lut_st *lutsid = &tree->lutsid;
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

void swicc_disk_lutid_empty(swicc_disk_st *const disk)
{
    if (disk == NULL)
    {
        return;
    }
    swicc_disk_lut_st *lutid = &disk->lutid;
    if (lutid->buf1 != NULL)
    {
        free(lutid->buf1);
    }
    if (lutid->buf2 != NULL)
    {
        free(lutid->buf2);
    }
    memset(&disk->lutid, 0U, sizeof(disk->lutid));
}

typedef struct lutid_rebuild_cb_userdata_s
{
    swicc_disk_lut_st *lut;
    uint8_t tree_idx;
} lutid_rebuild_cb_userdata_st;
/**
 * @brief Callback used when rebuilding the ID LUT. It receives files and
 * inserts their info into the ID LUT.
 * @param tree
 * @param file
 * @param userdata This must point to the userdata struct.
 */
static fs_file_foreach_cb lutid_rebuild_cb;
static swicc_ret_et lutid_rebuild_cb(swicc_disk_tree_st *const tree,
                                     swicc_fs_file_st *const file,
                                     void *const userdata)
{
    if (file->hdr_file.id == SWICC_FS_ID_MISSING)
    {
        return SWICC_RET_SUCCESS;
    }

    /* Insert the ID + offset into the ID LUT. */
    lutid_rebuild_cb_userdata_st const *const userdata_struct = userdata;
    swicc_disk_lut_st *const lutid = userdata_struct->lut;
    uint8_t entry_item1[sizeof(swicc_fs_id_kt)];
    /**
     * @note IDs are kept in big-endian inside the LUT so that they are sorted
     * from MSB to LSB.
     */
    swicc_fs_id_kt const id_be = htobe16(file->hdr_file.id);
    memcpy(entry_item1, &id_be, sizeof(swicc_fs_id_kt));
    uint8_t entry_item2[lutid->size_item2];
    memcpy(&entry_item2[0U], &file->hdr_item.offset_trel, sizeof(uint32_t));
    memcpy(&entry_item2[sizeof(uint32_t)], &userdata_struct->tree_idx,
           sizeof(uint8_t));
    return lut_insert(lutid, entry_item1, entry_item2);
}

swicc_ret_et swicc_disk_lutid_rebuild(swicc_disk_st *const disk)
{
    if (disk == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }
    swicc_ret_et ret = SWICC_RET_ERROR;
    /* Cleanup the old ID LUT before rebuilding it. */
    swicc_disk_lutid_empty(disk);
    disk->lutid.size_item1 = sizeof(swicc_fs_id_kt);
    disk->lutid.size_item2 =
        sizeof(uint32_t) + sizeof(uint8_t); /* Offset + tree index */
    disk->lutid.count_max = LUT_COUNT_START;
    disk->lutid.count = 0U;
    disk->lutid.buf1 = malloc(disk->lutid.count_max * disk->lutid.size_item1);
    disk->lutid.buf2 = malloc(disk->lutid.count_max * disk->lutid.size_item2);
    if (disk->lutid.buf1 == NULL || disk->lutid.buf2 == NULL)
    {
        swicc_disk_lutid_empty(disk);
        return SWICC_RET_ERROR;
    }

    swicc_disk_tree_st *tree = disk->root;
    uint8_t tree_idx = 0U;
    while (tree != NULL)
    {
        lutid_rebuild_cb_userdata_st userdata = {.lut = &disk->lutid,
                                                 .tree_idx = tree_idx};
        swicc_fs_file_st file_root;
        ret = swicc_disk_tree_file_root(tree, &file_root);
        if (ret != SWICC_RET_SUCCESS)
        {
            swicc_disk_lutid_empty(disk);
            break;
        }
        ret = swicc_disk_file_foreach(tree, &file_root, lutid_rebuild_cb,
                                      &userdata, true);
        if (ret != SWICC_RET_SUCCESS)
        {
            swicc_disk_lutid_empty(disk);
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
static swicc_ret_et lutsid_rebuild_cb(swicc_disk_tree_st *const tree,
                                      swicc_fs_file_st *const file,
                                      void *const userdata)
{
    if (file->hdr_file.sid == SWICC_FS_SID_MISSING)
    {
        return SWICC_RET_SUCCESS;
    }

    /* Insert the SID + offset into the SID LUT. */
    return lut_insert(&tree->lutsid, (uint8_t *)&file->hdr_file.sid,
                      (uint8_t *)&file->hdr_item.offset_trel);
}

swicc_ret_et swicc_disk_lutsid_rebuild(swicc_disk_st *const disk,
                                       swicc_disk_tree_st *const tree)
{
    if (disk == NULL || tree == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    /* Cleanup the old SID LUT before rebuilding it. */
    swicc_disk_lutsid_empty(tree);
    tree->lutsid.size_item1 = sizeof(swicc_fs_sid_kt);
    tree->lutsid.size_item2 = sizeof(uint32_t);
    tree->lutsid.count_max = LUT_COUNT_START;
    tree->lutsid.count = 0U;
    tree->lutsid.buf1 =
        malloc(tree->lutsid.count_max * tree->lutsid.size_item1);
    tree->lutsid.buf2 =
        malloc(tree->lutsid.count_max * tree->lutsid.size_item2);
    if (tree->lutsid.buf1 == NULL || tree->lutsid.buf2 == NULL)
    {
        swicc_disk_lutsid_empty(tree);
        return SWICC_RET_ERROR;
    }

    swicc_fs_file_st file_root;
    swicc_ret_et ret = swicc_disk_tree_file_root(tree, &file_root);
    if (ret != SWICC_RET_SUCCESS)
    {
        swicc_disk_lutsid_empty(tree);
        return ret;
    }
    ret = swicc_disk_file_foreach(tree, &file_root, lutsid_rebuild_cb, NULL,
                                  true);
    if (ret != SWICC_RET_SUCCESS)
    {
        swicc_disk_lutsid_empty(tree);
        return ret;
    }
    return ret;
}

swicc_ret_et swicc_disk_lutsid_lookup(swicc_disk_tree_st const *const tree,
                                      swicc_fs_sid_kt const sid,
                                      swicc_fs_file_st *const file)
{
    if (tree == NULL || file == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_disk_lut_st const *const lutsid = &tree->lutsid;
    /* Make sure the SID LUT is as expected. */
    if (lutsid->buf1 == NULL || lutsid->size_item1 != sizeof(swicc_fs_sid_kt) ||
        lutsid->size_item2 != sizeof(uint32_t))
    {
        return SWICC_RET_ERROR;
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
        return SWICC_RET_FS_NOT_FOUND;
    }

    uint32_t const offset =
        *(uint32_t *)&lutsid->buf2[lutsid->size_item2 * entry_idx];
    /* Offset too large. */
    if (offset >= tree->len)
    {
        return SWICC_RET_ERROR;
    }
    if (swicc_fs_file_prs(tree, offset, file) != SWICC_RET_SUCCESS)
    {
        return SWICC_RET_ERROR;
    }
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_disk_lutid_lookup(swicc_disk_st const *const disk,
                                     swicc_disk_tree_st **const tree,
                                     swicc_fs_id_kt const id,
                                     swicc_fs_file_st *const file)
{
    if (disk == NULL || tree == NULL || file == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_disk_lut_st const *const lutid = &disk->lutid;

    /* Make sure the ID LUT is as expected. */
    if (lutid->buf1 == NULL || lutid->size_item1 != sizeof(swicc_fs_id_kt) ||
        lutid->size_item2 != sizeof(uint32_t) + sizeof(uint8_t))
    {
        return SWICC_RET_ERROR;
    }

    /* Find the file by ID. */
    uint32_t entry_idx = 0U;
    while (entry_idx < lutid->count)
    {
        /* ID's are stored in big-endian inside the LUT. */
        swicc_fs_id_kt const id_be = htons(id);

        if (memcmp(&lutid->buf1[lutid->size_item1 * entry_idx], &id_be,
                   lutid->size_item1) == 0)
        {
            break;
        }
        entry_idx += 1U;
    }
    if (entry_idx >= lutid->count)
    {
        return SWICC_RET_FS_NOT_FOUND;
    }

    uint32_t const offset =
        *(uint32_t *)&lutid->buf2[(lutid->size_item2 * entry_idx) + 0U];
    uint8_t const tree_idx =
        lutid->buf2[(lutid->size_item2 * entry_idx) + sizeof(uint32_t)];

    /* Find the tree in which the file resides. */
    swicc_disk_tree_iter_st tree_iter;
    if (swicc_disk_tree_iter(disk, &tree_iter) != SWICC_RET_SUCCESS ||
        swicc_disk_tree_iter_idx(&tree_iter, tree_idx, tree) !=
            SWICC_RET_SUCCESS ||
        (*tree)->len <= offset)
    {
        return SWICC_RET_ERROR;
    }

    /* Offset too large. */
    if (offset >= (*tree)->len)
    {
        return SWICC_RET_ERROR;
    }
    if (swicc_fs_file_prs(*tree, offset, file) != SWICC_RET_SUCCESS)
    {
        return SWICC_RET_ERROR;
    }
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_disk_file_rcrd(swicc_disk_tree_st const *const tree,
                                  swicc_fs_file_st const *const file,
                                  swicc_fs_rcrd_idx_kt const idx,
                                  uint8_t **const buf, uint8_t *const len)
{
    if (tree == NULL || file == NULL || buf == NULL || len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    /* Only linear-fixed or cyclic files have records. */
    if (file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||
        file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        uint8_t const rcrd_size =
            file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED
                ? file->hdr_spec.ef_linearfixed.rcrd_size
                : file->hdr_spec.ef_cyclic.rcrd_size;

        uint32_t rcrd_cnt;
        swicc_ret_et const ret_rcrd_cnt =
            swicc_disk_file_rcrd_cnt(tree, file, &rcrd_cnt);
        if (ret_rcrd_cnt == SWICC_RET_SUCCESS)
        {
            if (idx >= rcrd_cnt)
            {
                return SWICC_RET_FS_NOT_FOUND;
            }
            /* Safe cast since uint8 * uint8 will fit in a uint32. */
            uint32_t const rcrd_offset = (uint32_t)(rcrd_size * idx);
            static_assert(
                sizeof(rcrd_size) == 1 && sizeof(idx) == 1,
                "Expected values to be 1 byte wide for cast to be safe");
            if (rcrd_offset >= file->data_size)
            {
                return SWICC_RET_ERROR;
            }
            *buf = &file->data[rcrd_offset];
            *len = rcrd_size;
            return SWICC_RET_SUCCESS;
        }
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_disk_file_rcrd_cnt(swicc_disk_tree_st const *const tree,
                                      swicc_fs_file_st const *const file,
                                      uint32_t *const rcrd_cnt)
{
    if (tree == NULL || file == NULL || rcrd_cnt == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    if (file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED)
    {
        /* Safe case since there can only be uint32_max records at most. */
        *rcrd_cnt = (uint32_t)(file->data_size /
                               file->hdr_spec.ef_linearfixed.rcrd_size);
        return SWICC_RET_SUCCESS;
    }
    else if (file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        /* Safe case since there can only be uint32_max records at most. */
        *rcrd_cnt =
            (uint32_t)((file->data_size) / file->hdr_spec.ef_cyclic.rcrd_size);
        return SWICC_RET_SUCCESS;
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_disk_tree_file_root(swicc_disk_tree_st const *const tree,
                                       swicc_fs_file_st *const file_root)
{
    if (tree == NULL || file_root == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    if (swicc_fs_file_prs(tree, 0U, file_root) == SWICC_RET_SUCCESS)
    {
        if (file_root->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_ADF ||
            file_root->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_MF)
        {
            return SWICC_RET_SUCCESS;
        }
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_disk_tree_file_parent(swicc_disk_tree_st const *const tree,
                                         swicc_fs_file_st const *const file,
                                         swicc_fs_file_st *const file_parent)
{
    swicc_ret_et ret = SWICC_RET_ERROR;
    int64_t const parent_offset_trel =
        file->hdr_item.offset_trel - file->hdr_item.offset_prel;
    if (parent_offset_trel >= 0U && parent_offset_trel <= UINT32_MAX)
    {
        return swicc_fs_file_prs(tree, (uint32_t)parent_offset_trel,
                                 file_parent);
    }
    return ret;
}

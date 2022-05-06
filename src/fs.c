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
 * @brief Find a tree by index in the forest/root.
 * @param disk
 * @param tree_idx
 * @param tree Where to write the pointer to the found tree. Only
 * valid on success.
 * @return Return code.
 */
__attribute__((unused)) static uicc_ret_et disk_root_tree_find(
    uicc_fs_disk_st *const disk, uint8_t const tree_idx,
    uicc_fs_tree_st **const tree)
{
    uint8_t tree_idx_cur = 0U;
    uicc_fs_tree_st *tree_tmp = disk->root;
    while (tree_tmp != NULL)
    {
        if (tree_idx == tree_idx_cur)
        {
            *tree = tree_tmp;
            return UICC_RET_SUCCESS;
        }
        tree_tmp = tree_tmp->next;

        /* Unsafe cast that relies on there being fewer than 256 trees. */
        tree_idx_cur = (uint8_t)(tree_idx_cur + 1U);
    }
    return UICC_RET_ERROR;
}

/**
 * @brief Insert an entry into a LUT (resizes the LUT if needed).
 * @param lut
 * @param entry_item1 This will be placed in buffer 1 and must have size equal
 * to the item size 1.
 * @param entry_item2 This will be placed in buffer 2 and must have size equal
 * to the item size 2.
 * @return Return code.
 */
static uicc_ret_et lut_insert(uicc_fs_lut_st *const lut,
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

/**
 * @brief A callback for the 'foreach' iterator which will run for every file in
 * a tree.
 * @param tree The tree inside which is the file.
 * @param file A file in the tree.
 * @param userdata Anything the user needs to access in their callback which is
 * not already provided.
 * @return Return code.
 */
typedef uicc_ret_et fs_tree_file_foreach_cb(uicc_fs_tree_st *const tree,
                                            uicc_fs_file_hdr_st *const file,
                                            void *const userdata);
/**
 * @brief For every file in a tree, perform some operation.
 * @param tree Tree to iterate through all items in.
 * @param cb A callback that will be run for every item in the tree.
 * @param userdata Pointer to any additional data the user needs access to in
 * the callback.
 * @return Return code.
 */
static uicc_ret_et fs_tree_file_foreach(uicc_fs_tree_st *const tree,
                                        fs_tree_file_foreach_cb *const cb,
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
                sizeof(uicc_fs_file_hdr_raw_st), 0};
            uint32_t depth = 1U; /* Inside the tree so 1 already. */
            while (depth < UICC_FS_DEPTH_MAX)
            {
                if (stack_data_idx[depth] >= file_hdr.item.size)
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
                    if (stack_data_idx[depth - 1U] +
                            sizeof(*file_nested_hdr_raw) >
                        UINT32_MAX)
                    {
                        /* Data index would overflow. */
                        uicc_fs_disk_lutsid_empty(tree);
                        ret = UICC_RET_ERROR;
                        break;
                    }
                    /* Safe cast due to overflow check. */
                    stack_data_idx[depth - 1U] =
                        (uint32_t)(stack_data_idx[depth - 1U] +
                                   sizeof(*file_nested_hdr_raw));
                    break;
                case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
                case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
                case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
                    stack_data_idx[depth - 1U] += file_nested_hdr.item.size;
                    break;
                default:
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

uicc_ret_et uicc_fs_reset(uicc_st *const uicc_state)
{
    memset(&uicc_state->internal.fs.va, 0U, sizeof(uicc_fs_va_st));
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_fs_disk_load(uicc_st *const uicc_state,
                              char const *const disk_path)
{
    uicc_ret_et ret = UICC_RET_ERROR;
    if (uicc_state->internal.fs.disk.root != NULL)
    {
        /* Get rid of the current disk first before loading a new one. */
        return ret;
    }

    uicc_fs_disk_st *disk = &uicc_state->internal.fs.disk;
    /* Clear disk so that all the members have a known initial state. */
    memset(disk, 0U, sizeof(*disk));

    FILE *f = fopen(disk_path, "r");
    if (!(f == NULL))
    {
        if (fseek(f, 0, SEEK_END) == 0)
        {
            int64_t f_len = ftell(f);
            if (f_len >= 0 && f_len <= UINT32_MAX)
            {
                if (fseek(f, 0, SEEK_SET) == 0)
                {
                    uint8_t magic_expected[UICC_FS_MAGIC_LEN] = UICC_FS_MAGIC;
                    uint8_t magic[UICC_FS_MAGIC_LEN];
                    if (fread(&magic, UICC_FS_MAGIC_LEN, 1U, f) == 1U)
                    {
                        if (memcmp(magic, magic_expected, UICC_FS_MAGIC_LEN) ==
                            0)
                        {
                            /**
                             * Parse the root (forest of trees) contained in the
                             * file.
                             */
                            uicc_fs_tree_st *tree = NULL;
                            uicc_ret_et ret_item = UICC_RET_SUCCESS;
                            uint32_t data_idx = UICC_FS_MAGIC_LEN;
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
                                if (item_hdr.type == UICC_FS_ITEM_TYPE_INVALID)
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
        uicc_fs_disk_root_empty(&uicc_state->internal.fs.disk);
    }
    else
    {
        /* Create all the LUTs. */
        uicc_fs_tree_st *tree = disk->root;
        if (tree == NULL)
        {
            ret = UICC_RET_ERROR;
        }
        while (tree != NULL)
        {
            ret = uicc_fs_lutsid_rebuild(disk, tree);
            if (ret != UICC_RET_SUCCESS)
            {
                break;
            }
            tree = tree->next;
        }
        if (ret == UICC_RET_SUCCESS)
        {
            ret = uicc_fs_lutid_rebuild(disk);
        }
    }
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_fs_disk_root_empty(&uicc_state->internal.fs.disk);
    }
    return ret;
}

void uicc_fs_disk_unload(uicc_st *const uicc_state)
{
    /* This also frees the SID LUT of all trees. */
    uicc_fs_disk_root_empty(&uicc_state->internal.fs.disk);

    uicc_fs_disk_lutid_empty(&uicc_state->internal.fs.disk);
    memset(&uicc_state->internal.fs, 0U, sizeof(uicc_state->internal.fs));
}

void uicc_fs_disk_root_empty(uicc_fs_disk_st *const disk)
{
    uicc_fs_tree_st *tree = disk->root;
    while (tree != NULL)
    {
        if (tree->buf != NULL)
        {
            free(tree->buf);
        }

        /* Free the SID LUT of this tree. */
        uicc_fs_disk_lutsid_empty(tree);

        uicc_fs_tree_st *const tree_next = tree->next;
        free(tree);
        tree = tree_next;
    }
    disk->root = NULL;
}

void uicc_fs_disk_lutsid_empty(uicc_fs_tree_st *const tree)
{
    uicc_fs_lut_st *lutsid = &tree->lutsid;
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

void uicc_fs_disk_lutid_empty(uicc_fs_disk_st *const disk)
{
    uicc_fs_lut_st *lutid = &disk->lutid;
    free(lutid->buf1);
    free(lutid->buf2);
    memset(&disk->lutid, 0U, sizeof(disk->lutid));
}

uicc_ret_et uicc_fs_disk_save(uicc_fs_disk_st *const disk,
                              char const *const disk_path)
{
    uicc_ret_et ret = UICC_RET_ERROR;
    FILE *f = fopen(disk_path, "w");
    if (f != NULL)
    {
        uint8_t magic[UICC_FS_MAGIC_LEN] = UICC_FS_MAGIC;
        if (fwrite(magic, UICC_FS_MAGIC_LEN, 1U, f) == 1U)
        {
            uicc_fs_tree_st *tree = disk->root;
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

uicc_ret_et uicc_fs_item_hdr_prs(
    uicc_fs_item_hdr_raw_st const *const item_hdr_raw,
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
    default:
        item_hdr->type = UICC_FS_ITEM_TYPE_INVALID;
    };
    item_hdr->lcs = item_hdr_raw->lcs;
    item_hdr->size = item_hdr_raw->size;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_fs_file_hdr_prs(
    uicc_fs_file_hdr_raw_st const *const file_hdr_raw,
    uicc_fs_file_hdr_st *const file_hdr)
{
    file_hdr->id = file_hdr_raw->id;
    file_hdr->sid = file_hdr_raw->sid;
    memcpy((void *)file_hdr->name, file_hdr_raw->name, UICC_FS_NAME_LEN_MAX);
    file_hdr->name[UICC_FS_NAME_LEN_MAX] = '\0';
    return UICC_RET_SUCCESS;
}

static fs_tree_file_foreach_cb lutid_rebuild_cb;
typedef struct lut_id_rebuild_cb_userdata_s
{
    uicc_fs_lut_st *lut;
    uint8_t tree_idx;
} lut_id_rebuild_cb_userdata_st;
static uicc_ret_et lutid_rebuild_cb(uicc_fs_tree_st *const tree,
                                    uicc_fs_file_hdr_st *const file,
                                    void *const userdata)
{
    if (file->id == UICC_FS_ID_MISSING)
    {
        return UICC_RET_SUCCESS;
    }

    /* Insert the ID + offset into the ID LUT. */
    lut_id_rebuild_cb_userdata_st const *const userdata_struct = userdata;
    uicc_fs_lut_st *const lutid = userdata_struct->lut;
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

uicc_ret_et uicc_fs_lutid_rebuild(uicc_fs_disk_st *const disk)
{
    uicc_ret_et ret = UICC_RET_ERROR;

    /* Cleanup the old ID LUT before rebuilding it. */
    uicc_fs_disk_lutid_empty(disk);
    disk->lutid.size_item1 = sizeof(uicc_fs_id_kt);
    disk->lutid.size_item2 =
        sizeof(uint8_t) + sizeof(uint32_t); /* Tree index + Offset */
    disk->lutid.count_max = LUT_COUNT_START;
    disk->lutid.count = 0U;
    disk->lutid.buf1 = malloc(disk->lutid.count_max * disk->lutid.size_item1);
    disk->lutid.buf2 = malloc(disk->lutid.count_max * disk->lutid.size_item2);
    if (disk->lutid.buf1 == NULL || disk->lutid.buf2 == NULL)
    {
        uicc_fs_disk_lutid_empty(disk);
        return UICC_RET_ERROR;
    }

    uicc_fs_tree_st *tree = disk->root;
    uint8_t tree_idx = 0U;
    while (tree != NULL)
    {
        lut_id_rebuild_cb_userdata_st userdata = {.lut = &disk->lutid,
                                                  .tree_idx = tree_idx};
        ret = fs_tree_file_foreach(tree, lutid_rebuild_cb, &userdata);
        if (ret != UICC_RET_SUCCESS)
        {
            uicc_fs_disk_lutid_empty(disk);
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

static fs_tree_file_foreach_cb lutsid_rebuild_cb;
static uicc_ret_et lutsid_rebuild_cb(uicc_fs_tree_st *const tree,
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

uicc_ret_et uicc_fs_lutsid_rebuild(uicc_fs_disk_st *const disk,
                                   uicc_fs_tree_st *const tree)
{
    /* Cleanup the old SID LUT before rebuilding it. */
    uicc_fs_disk_lutsid_empty(tree);
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
        uicc_fs_disk_lutsid_empty(tree);
        return UICC_RET_ERROR;
    }

    uicc_ret_et ret = fs_tree_file_foreach(tree, lutsid_rebuild_cb, NULL);
    if (ret != UICC_RET_SUCCESS)
    {
        uicc_fs_disk_lutsid_empty(tree);
    }
    return ret;
}

uicc_ret_et uicc_fs_select_file_dfname(uicc_st *const uicc_state,
                                       char const *const df_name,
                                       uint32_t const df_name_len)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_file_fid(uicc_st *const uicc_state,
                                    uicc_fs_id_kt const fid)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_file_path(uicc_st *const uicc_state,
                                     char const *const path,
                                     uint32_t const path_len)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_do_tag(uicc_st *const uicc_state, uint16_t const tag)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_record_id(uicc_st *const uicc_state,
                                     uicc_fs_rcrd_id_kt id)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_record_idx(uicc_st *const uicc_state,
                                      uicc_fs_rcrd_idx_kt idx)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_data_offset(uicc_st *const uicc_state,
                                       uint32_t offset_prel)
{
    return UICC_RET_UNKNOWN;
}

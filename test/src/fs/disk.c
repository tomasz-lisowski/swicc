#include <tau/tau.h>

#include <cJSON.h>
#include <swicc/swicc.h>

static int32_t filesize(char const *const path, uint32_t *const size)
{
    FILE *f = fopen(path, "r");
    int32_t err = fseek(f, 0U, SEEK_END);
    if (err != 0)
    {
        return err;
    }
    int64_t f_size = ftell(f);
    if (f_size < 0 || f_size > UINT32_MAX)
    {
        return -1;
    }
    /* Safe cast since file size was checked to fit in uint32 range. */
    *size = (uint32_t)f_size;
    fclose(f);
    return 0;
}

/**
 * @warning The static function 'lut_insert' is not tested because it's static.
 */

TEST(fs_disk, swicc_disk_save__param_check)
{
    swicc_disk_st *const disk = (swicc_disk_st *)1U;
    char const *const disk_path = "";
    CHECK_EQ(swicc_disk_save(NULL, disk_path), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_save(disk, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_save__disk)
{
    uint32_t disk_filesize;
    if (filesize("test/data/disk/004-out.bin", &disk_filesize) == 0)
    {
        uint8_t disk_buf[disk_filesize];
        memset(disk_buf, 0U, disk_filesize);

        char const *const disk_path = "build/tmp/3LkgKmhtzapMe5bz.swiccfs";
        swicc_disk_st disk = {0U};
        REQUIRE_EQ(
            swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
            SWICC_RET_SUCCESS);
        uint32_t const lutid_size =
            (disk.lutid.size_item1 + disk.lutid.size_item2) * disk.lutid.count;
        CHECK_EQ(swicc_disk_save(&disk, disk_path), SWICC_RET_SUCCESS);
        FILE *const fdisk = fopen(disk_path, "rb");
        CHECK_EQ(fread(disk_buf, disk_filesize - lutid_size, 1U, fdisk), 1U);
        if (fclose(fdisk) != 0)
        {
            WARN("Disk file failed to close.");
        }
        swicc_disk_unload(&disk);
    }
    else
    {
        WARN("Failed to get file size.");
    }
}

TEST(fs_disk, swicc_disk_load__param_check)
{
    swicc_disk_st *const disk = (swicc_disk_st *)1U;
    char const *const disk_path = "";
    CHECK_EQ(swicc_disk_load(NULL, disk_path), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_load(disk, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_load__disk)
{
    uint32_t disk_filesize;
    if (filesize("test/data/disk/004-out.bin", &disk_filesize) == 0)
    {
        uint8_t disk_buf[disk_filesize];
        memset(disk_buf, 0U, sizeof(disk_buf));

        char const *const disk_path0 = "build/tmp/Z0fnnIBCykwwaiLJ.swiccfs";
        char const *const disk_path1 = "build/tmp/mP2F0I8XMnzLhBzR.swiccfs";
        swicc_disk_st disk = {0U};
        REQUIRE_EQ(
            swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
            SWICC_RET_SUCCESS);
        uint32_t const lutid_size =
            (disk.lutid.size_item1 + disk.lutid.size_item2) * disk.lutid.count;
        CHECK_EQ(swicc_disk_save(&disk, disk_path0), SWICC_RET_SUCCESS);
        swicc_disk_unload(&disk);
        CHECK_EQ(swicc_disk_load(&disk, disk_path0), SWICC_RET_SUCCESS);
        CHECK_EQ(swicc_disk_save(&disk, disk_path1), SWICC_RET_SUCCESS);
        FILE *const fdisk = fopen(disk_path1, "rb");
        CHECK_EQ(fread(disk_buf, disk_filesize - lutid_size, 1U, fdisk), 1U);
        if (fclose(fdisk) != 0)
        {
            WARN("Disk file failed to close.");
        }
        swicc_disk_unload(&disk);
    }
    else
    {
        WARN("Failed to get file size.");
    }
}

TEST(fs_disk, swicc_disk_unload__disk)
{
    swicc_disk_st const disk_zero = {0U};
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_unload(&disk);
    CHECK_BUF_EQ(&disk, &disk_zero, sizeof(disk));
}

TEST(fs_disk, swicc_disk_file_foreach__param_check)
{
    /* These are invalid pointers but they are not NULL. */
    swicc_disk_tree_st *const tree = (swicc_disk_tree_st *)1U;
    swicc_fs_file_st *const file = (swicc_fs_file_st *)1U;
    swicc_disk_file_foreach_cb *const cb = (swicc_disk_file_foreach_cb *)1U;
    void *const userdata = (void *)1U;
    CHECK_EQ(swicc_disk_file_foreach(NULL, file, cb, userdata, true),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_foreach(tree, NULL, cb, userdata, true),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_foreach(tree, file, NULL, userdata, true),
             SWICC_RET_PARAM_BAD);
}

typedef struct swicc_disk_file_foreach__disk_userdata_s
{
    uint32_t file_count;
    uint32_t valid_count;
} swicc_disk_file_foreach__disk_userdata_st;
static swicc_disk_file_foreach_cb swicc_disk_file_foreach__disk_cb;
static swicc_ret_et swicc_disk_file_foreach__disk_cb(
    swicc_disk_tree_st *const tree, swicc_fs_file_st *const file,
    void *const userdata)
{
    swicc_disk_file_foreach__disk_userdata_st *const data = userdata;
    bool valid = false;
    uint32_t const hdr_size = swicc_fs_item_hdr_raw_size[file->hdr_item.type];
    uint32_t file_size;
    /* Expecting 3 files. */
    switch (data->file_count)
    {
    case 0:
        file_size = 87U;
        valid = file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_MF &&
                file->hdr_item.lcs == SWICC_FS_LCS_OPER_ACTIV &&
                file->hdr_item.offset_trel == 0U &&
                file->hdr_item.offset_prel == 0U &&
                file->hdr_item.size == file_size &&
                file->hdr_file.id == 0x64F3 && file->hdr_file.sid == 0x22 &&
                memcmp(file->hdr_spec.mf.name, "HbviIoQeXWVoAnpN",
                       sizeof(file->hdr_spec.mf.name)) == 0U &&
                file->internal.hdr_raw == tree->buf &&
                file->data == &file->internal.hdr_raw[hdr_size] &&
                file->data_size == file_size - hdr_size;
        break;
    case 1:
        file_size = 29U;
        valid = file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT &&
                file->hdr_item.lcs == SWICC_FS_LCS_OPER_ACTIV &&
                file->hdr_item.offset_trel == 29U &&
                file->hdr_item.offset_prel == 29U &&
                file->hdr_item.size == file_size &&
                file->hdr_file.id == 0x15B4 && file->hdr_file.sid == 0xCB &&
                file->internal.hdr_raw == &tree->buf[29U] &&
                file->data == &file->internal.hdr_raw[hdr_size] &&
                file->data_size == file_size - hdr_size;
        break;
    case 2:
        file_size = 29U;
        valid = file->hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT &&
                file->hdr_item.lcs == SWICC_FS_LCS_OPER_ACTIV &&
                file->hdr_item.offset_trel == 58U &&
                file->hdr_item.offset_prel == 58U &&
                file->hdr_item.size == file_size &&
                file->hdr_file.id == 0x37CC && file->hdr_file.sid == 0x8D &&
                file->internal.hdr_raw == &tree->buf[58U] &&
                file->data == &file->internal.hdr_raw[hdr_size] &&
                file->data_size == file_size - hdr_size;
        break;
    }
    if (valid)
    {
        data->valid_count += 1U;
    }
    data->file_count += 1U;
    return SWICC_RET_SUCCESS;
}

TEST(fs_disk, swicc_disk_file_foreach__disk)
{
    swicc_disk_file_foreach__disk_userdata_st data = {.file_count = 0U,
                                                      .valid_count = 0U};
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_fs_file_st file_root;
    swicc_ret_et const ret_root =
        swicc_disk_tree_file_root(disk.root, &file_root);
    if (ret_root == SWICC_RET_SUCCESS)
    {

        CHECK_EQ(swicc_disk_file_foreach(disk.root, &file_root,
                                         swicc_disk_file_foreach__disk_cb,
                                         &data, true),
                 SWICC_RET_SUCCESS);
        CHECK_EQ(data.valid_count, data.file_count);
    }
    else
    {
        WARN("Failed to get the root file of the root tree.");
    }
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_tree_iter__param_check)
{
    swicc_disk_st *const disk = (swicc_disk_st *)1U;
    swicc_disk_tree_iter_st *const tree_iter = (swicc_disk_tree_iter_st *)1U;
    CHECK_EQ(swicc_disk_tree_iter(NULL, tree_iter), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_tree_iter(disk, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_tree_iter__disk)
{
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    CHECK_EQ(tree_iter.tree_idx, 0U);
    CHECK_EQ((void *)tree_iter.tree, (void *)disk.root);
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_tree_iter_next__param_check)
{
    swicc_disk_tree_iter_st *const tree_iter = (swicc_disk_tree_iter_st *)1U;
    swicc_disk_tree_st **const tree = (swicc_disk_tree_st **)1U;
    CHECK_EQ(swicc_disk_tree_iter_next(NULL, tree), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_tree_iter_next(tree_iter, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_tree_iter_next__disk)
{
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    CHECK_EQ(swicc_disk_tree_iter_next(&tree_iter, &tree), SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root->next);
    CHECK_EQ(swicc_disk_tree_iter_next(&tree_iter, &tree),
             SWICC_RET_FS_NOT_FOUND);
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_tree_iter_idx__param_check)
{
    swicc_disk_tree_iter_st *const tree_iter = (swicc_disk_tree_iter_st *)1U;
    swicc_disk_tree_st **const tree = (swicc_disk_tree_st **)1U;
    CHECK_EQ(swicc_disk_tree_iter_idx(NULL, 0U, tree), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_tree_iter_idx(tree_iter, 0U, NULL),
             SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_tree_iter_idx__disk_ordered)
{
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/005-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 0U, &tree),
             SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root);
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 1U, &tree),
             SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root->next);
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 2U, &tree),
             SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root->next->next);
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 3U, &tree),
             SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root->next->next->next);
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 4U, &tree),
             SWICC_RET_FS_NOT_FOUND);
    CHECK_EQ((void *)tree, (void *)disk.root->next->next->next);
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_tree_iter_idx__disk_unordered)
{
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/005-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 2U, &tree),
             SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root->next->next);
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 1U, &tree),
             SWICC_RET_FS_NOT_FOUND);
    CHECK_EQ((void *)tree, (void *)disk.root->next->next->next);
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    CHECK_EQ(swicc_disk_tree_iter_idx(&tree_iter, 3U, &tree),
             SWICC_RET_SUCCESS);
    CHECK_EQ((void *)tree, (void *)disk.root->next->next->next);
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_root_empty__disk)
{
    swicc_disk_lut_st const lutid_zero = {0};
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_root_empty(&disk);
    CHECK_EQ((void *)disk.root, NULL);
    CHECK_BUF_EQ((uint8_t *)&disk.lutid, (uint8_t *)&lutid_zero,
                 sizeof(lutid_zero));
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_lutsid_empty__disk)
{
    swicc_disk_lut_st const lutsid_zero = {0};
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_lutsid_empty(disk.root);
    CHECK_BUF_EQ((uint8_t *)&disk.root->lutsid, (uint8_t *)&lutsid_zero,
                 sizeof(lutsid_zero));
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_lutid_empty__disk)
{
    swicc_disk_lut_st const lutid_zero = {0};
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_lutid_empty(&disk);
    CHECK_BUF_EQ((uint8_t *)&disk.lutid, (uint8_t *)&lutid_zero,
                 sizeof(lutid_zero));
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_lutid_rebuild__param_check)
{
    CHECK_EQ(swicc_disk_lutid_rebuild(NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_lutid_rebuild__disk)
{
    swicc_disk_lut_st lutid_copy;
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);

    /* Move the ID LUT out of the disk into a local var. */
    memcpy(&lutid_copy, &disk.lutid, sizeof(swicc_disk_lut_st));
    memset(&disk.lutid, 0U, sizeof(swicc_disk_lut_st));

    /* Rebuild ID LUT in place of the one that was moved out. */
    CHECK_EQ(swicc_disk_lutid_rebuild(&disk), SWICC_RET_SUCCESS);

    /* Compare ID LUT contents. */
    CHECK_EQ(disk.lutid.count, lutid_copy.count);
    CHECK_EQ(disk.lutid.count_max, lutid_copy.count_max);
    CHECK_EQ(disk.lutid.size_item1, lutid_copy.size_item1);
    CHECK_EQ(disk.lutid.size_item2, lutid_copy.size_item2);
    int32_t const buf1_size =
        (int32_t)(disk.lutid.count * disk.lutid.size_item1);
    int32_t const buf2_size =
        (int32_t)(disk.lutid.count * disk.lutid.size_item2);
    CHECK_BUF_EQ(disk.lutid.buf1, lutid_copy.buf1, (size_t)buf1_size);
    CHECK_BUF_EQ(disk.lutid.buf2, lutid_copy.buf2, (size_t)buf2_size);

    /* Unload first ID LUT. */
    swicc_disk_unload(&disk);

    /* Unload second ID LUT. */
    memcpy(&disk.lutid, &lutid_copy, sizeof(swicc_disk_lut_st));
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_lutsid_rebuild__param_check)
{
    swicc_disk_st *const disk = (swicc_disk_st *)1U;
    swicc_disk_tree_st *const tree = (swicc_disk_tree_st *)1U;
    CHECK_EQ(swicc_disk_lutsid_rebuild(NULL, tree), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_lutsid_rebuild(disk, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_lutsid_rebuild__disk)
{
    swicc_disk_lut_st lutsid_copy;
    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/004-in.json"),
               SWICC_RET_SUCCESS);

    /* Move the SID created at disk creation into a local var. */
    memcpy(&lutsid_copy, &disk.root->lutsid, sizeof(swicc_disk_lut_st));
    memset(&disk.root->lutsid, 0U, sizeof(swicc_disk_lut_st));

    /* Rebuild SID LUT in place of the one that was moved out. */
    CHECK_EQ(swicc_disk_lutsid_rebuild(&disk, disk.root), SWICC_RET_SUCCESS);

    /* Compare SID LUT contents. */
    CHECK_EQ(disk.root->lutsid.count, lutsid_copy.count);
    CHECK_EQ(disk.root->lutsid.count_max, lutsid_copy.count_max);
    CHECK_EQ(disk.root->lutsid.size_item1, lutsid_copy.size_item1);
    CHECK_EQ(disk.root->lutsid.size_item2, lutsid_copy.size_item2);
    int32_t const buf1_size =
        (int32_t)(disk.root->lutsid.count * disk.root->lutsid.size_item1);
    int32_t const buf2_size =
        (int32_t)(disk.root->lutsid.count * disk.root->lutsid.size_item2);
    CHECK_BUF_EQ(disk.root->lutsid.buf1, lutsid_copy.buf1, (size_t)buf1_size);
    CHECK_BUF_EQ(disk.root->lutsid.buf2, lutsid_copy.buf2, (size_t)buf2_size);

    /* Unload first SID LUT. */
    swicc_disk_unload(&disk);

    /* Unload second SID LUT. */
    memcpy(&disk.lutid, &lutsid_copy, sizeof(swicc_disk_lut_st));
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_lutsid_lookup__param_check)
{
    swicc_disk_tree_st *const tree = (swicc_disk_tree_st *)1U;
    swicc_fs_file_st *const file = (swicc_fs_file_st *)1U;
    CHECK_EQ(swicc_disk_lutsid_lookup(NULL, 0U, file), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_lutsid_lookup(tree, 0U, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_lutsid_lookup__disk)
{
    /**
     * @warning These need to be sorted.
     */
    static swicc_fs_id_kt const sid_valid[4U][4U] = {
        {0x71, 0x95, 0xB0, 0xC7},
        {0x44, 0x77, 0x7E, 0xCD},
        {0x61, 0x93, 0xA9, 0xCF},
        {0x63, 0xAD, 0xCF, 0xDD},
    };

    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/006-in.json"),
               SWICC_RET_SUCCESS);
    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    while (swicc_disk_tree_iter_next(&tree_iter, &tree) == SWICC_RET_SUCCESS)
    {
        uint16_t sid_idx = 0U;

        for (swicc_fs_sid_kt sid = 0U;; ++sid)
        {
            swicc_fs_file_st file;
            swicc_ret_et const ret_lookup =
                swicc_disk_lutsid_lookup(tree, sid, &file);
            swicc_ret_et ret_lookup_exp;
            char *info_msg_str = "???";

            if (sid_idx < sizeof(sid_valid[0U]) / sizeof(sid_valid[0U][0U]) &&
                sid == sid_valid[tree_iter.tree_idx][sid_idx])
            {
                /**
                 * Safe cast since sid index will grow upto the number of files
                 * in the tree which is less than uint16 max.
                 */
                sid_idx = (uint16_t)(sid_idx + 1U);
                ret_lookup_exp = SWICC_RET_SUCCESS;
                info_msg_str = "succeeded";
            }
            else
            {
                ret_lookup_exp = SWICC_RET_FS_NOT_FOUND;
                info_msg_str = "failed";
            }

            CHECK_EQ(ret_lookup, ret_lookup_exp);
            if (ret_lookup != ret_lookup_exp)
            {
                tauColouredPrintf(TAU_COLOUR_BRIGHTBLUE_, "[   INFO   ] ");
                tauColouredPrintf(
                    TAU_COLOUR_DEFAULT_,
                    "Lookup of 0x%02X in tree %u should have %s.\n", sid,
                    tree_iter.tree_idx, info_msg_str);
            }
            if (ret_lookup == SWICC_RET_SUCCESS)
            {
                /**
                 * Make sure the actual file that was retrieved has the correct
                 * SID.
                 */
                CHECK_EQ(file.hdr_file.sid, sid);
            }

            /* Prevents infinite loop. */
            if (sid == 0xFF)
            {
                break;
            }
        }
    }
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_lutid_lookup__param_check)
{
    swicc_disk_st *const disk = (swicc_disk_st *)1U;
    swicc_disk_tree_st **const tree = (swicc_disk_tree_st **)1U;
    swicc_fs_file_st *const file = (swicc_fs_file_st *)1U;
    CHECK_EQ(swicc_disk_lutid_lookup(NULL, tree, 0U, file),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_lutid_lookup(disk, NULL, 0U, file),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_lutid_lookup(disk, tree, 0U, NULL),
             SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_lutid_lookup__disk)
{
    static swicc_fs_id_kt const id_valid[] = {
        0x1217, 0x1704, 0x40B1, 0x5240, 0x5ABD, 0x60D6, 0x862F, 0x89E7,
        0x8EB4, 0xAF09, 0xB1E9, 0xC24A, 0xC3C0, 0xE7C7, 0xE99D, 0xF4F4,
    };

    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/006-in.json"),
               SWICC_RET_SUCCESS);

    uint16_t id_idx = 0U;
    for (swicc_fs_id_kt id = 0U;; ++id)
    {
        swicc_fs_file_st file;
        swicc_disk_tree_st *tree;
        swicc_ret_et const ret_lookup =
            swicc_disk_lutid_lookup(&disk, &tree, id, &file);
        swicc_ret_et ret_lookup_exp;
        char *info_msg_str = "???";

        if (id_idx < sizeof(id_valid) / sizeof(id_valid[0U]) &&
            id == id_valid[id_idx])
        {
            /**
             * Safe cast since sid index will grow upto the number of files
             * which is less than uint16 max.
             */
            id_idx = (uint16_t)(id_idx + 1U);
            ret_lookup_exp = SWICC_RET_SUCCESS;
            info_msg_str = "succeeded";
        }
        else
        {
            ret_lookup_exp = SWICC_RET_FS_NOT_FOUND;
            info_msg_str = "failed";
        }

        CHECK_EQ(ret_lookup, ret_lookup_exp);
        if (ret_lookup != ret_lookup_exp)
        {
            tauColouredPrintf(TAU_COLOUR_BRIGHTBLUE_, "[   INFO   ] ");
            tauColouredPrintf(TAU_COLOUR_DEFAULT_,
                              "Lookup of 0x%04X should have %s.\n", id,
                              info_msg_str);
        }
        if (ret_lookup == SWICC_RET_SUCCESS)
        {
            /**
             * Make sure the actual file that was retrieved has the correct ID
             * and is in the correct tree.
             */
            CHECK_EQ(file.hdr_file.id, id);
        }

        /* Prevents infinite loop. */
        if (id == 0xFFFF)
        {
            break;
        }
    }
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_file_rcrd__param_check)
{
    swicc_disk_tree_st *const tree = (swicc_disk_tree_st *)1U;
    swicc_fs_file_st *const file = (swicc_fs_file_st *)1U;
    uint8_t **const buf = (uint8_t **)1U;
    uint8_t *const len = (uint8_t *)1U;
    CHECK_EQ(swicc_disk_file_rcrd(NULL, file, 0U, buf, len),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_rcrd(tree, NULL, 0U, buf, len),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_rcrd(tree, file, 0U, NULL, len),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_rcrd(tree, file, 0U, buf, NULL),
             SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_file_rcrd__disk)
{
    /* Tree -> File */
    static swicc_fs_id_kt const file_rcrd_id[4U][2U] = {
        {0xE99D, 0x5ABD},
        {0x5240, 0xB1E9},
        {0x89E7, 0xC24A},
        {0x8EB4, 0xAF09},
    };
    /* Tree -> File -> Record -> Byte */
    static uint8_t const rcrd_data[4U][2U][3U][16U] = {
        {
            {
                {0xF6, 0x72, 0xFF, 0x99, 0x3B, 0x80, 0x83, 0x0F, 0xAE, 0xEE,
                 0xC9, 0x58, 0x84, 0xDC, 0x99, 0xE5},
                {0x46, 0xF1, 0x98, 0xE1, 0x4E, 0x67, 0x8E, 0x14, 0xB0, 0xF1,
                 0xFA, 0xDA, 0xB1, 0x9E, 0x13, 0xEC},
                {0xCF, 0x00, 0x28, 0xE5, 0x67, 0x3C, 0x5D, 0x1A, 0xA4, 0xC9,
                 0x16, 0x0B, 0x63, 0x38, 0x49, 0x29},
            },
            {
                {0xC4, 0x11, 0xA9, 0xC1, 0x07, 0xC9, 0xE8, 0x4D, 0xF3, 0xC7,
                 0x8D, 0xB5, 0x9E, 0xD3, 0xDD, 0x57},
                {0x4F, 0x76, 0x13, 0xD2, 0x66, 0x5B, 0x28, 0x2C, 0x54, 0xC7,
                 0x1D, 0x23, 0xB1, 0x41, 0x07, 0x12},
                {0xCC, 0x3E, 0x32, 0x1E, 0xC6, 0xCB, 0xA4, 0xFB, 0x34, 0xEC,
                 0x41, 0x16, 0x6A, 0x98, 0xFA, 0xE5},
            },
        },
        {
            {
                {0xF2, 0xB7, 0xCE, 0xF6, 0x95, 0xDD, 0x1B, 0x1E, 0x24, 0x71,
                 0x7A, 0x91, 0x3A, 0x1A, 0xE9, 0xEF},
                {0x08, 0x13, 0x47, 0xDE, 0xD7, 0x57, 0x2D, 0x63, 0xFE, 0xA4,
                 0x4D, 0x38, 0x39, 0xCB, 0xF9, 0x32},
                {0x1E, 0x42, 0x71, 0x91, 0x04, 0x12, 0x42, 0x88, 0x07, 0xEA,
                 0xA0, 0x6B, 0x3E, 0x17, 0xC7, 0x25},
            },
            {
                {0x1B, 0xCA, 0xC4, 0xA4, 0x18, 0xC1, 0xF8, 0x2E, 0x71, 0x52,
                 0xD9, 0xC3, 0x66, 0xF2, 0xDA, 0xA9},
                {0xB9, 0x18, 0xC6, 0x1C, 0xC4, 0xD0, 0x78, 0xB6, 0x9C, 0x87,
                 0x60, 0x16, 0x8D, 0x58, 0x27, 0x19},
                {0x51, 0xC9, 0xCA, 0xAB, 0x87, 0xAF, 0x61, 0x78, 0xC2, 0x75,
                 0x3F, 0x6B, 0x6F, 0x7F, 0x47, 0x8B},
            },
        },
        {
            {
                {0x49, 0x19, 0x7F, 0x30, 0xC6, 0x2A, 0x37, 0x6B, 0xBE, 0xD9,
                 0xE5, 0x7F, 0x78, 0xF9, 0x5B, 0x78},
                {0x8F, 0xE8, 0xF8, 0x81, 0x36, 0x7E, 0xE4, 0x63, 0x5F, 0x4B,
                 0xE0, 0xB6, 0x8F, 0xCD, 0x5B, 0xAA},
                {0x08, 0x59, 0x39, 0x5B, 0x6F, 0x8D, 0xDF, 0x90, 0x8C, 0xBA,
                 0x16, 0x5E, 0xF5, 0x4D, 0xE0, 0x50},
            },
            {
                {0x11, 0x79, 0xAC, 0x29, 0xFD, 0x13, 0x04, 0xF1, 0xEF, 0xCD,
                 0x36, 0xA8, 0x92, 0x34, 0x3B, 0x0D},
                {0x08, 0xD4, 0x8E, 0x2F, 0x62, 0x9A, 0x5A, 0x3D, 0xDC, 0xA1,
                 0x1A, 0x4A, 0x17, 0x3C, 0xE1, 0x18},
                {0xE7, 0x8D, 0x47, 0x05, 0x9E, 0x65, 0x10, 0x00, 0x1D, 0x2B,
                 0x41, 0xE0, 0x3C, 0xE5, 0xED, 0x74},
            },
        },
        {
            {
                {0x73, 0xCA, 0x44, 0xE3, 0xA8, 0x5D, 0x73, 0x36, 0x25, 0x13,
                 0xE7, 0xA1, 0x32, 0x16, 0x78, 0x11},
                {0x19, 0x27, 0x2E, 0x4B, 0xF2, 0x4A, 0xA8, 0x85, 0xE8, 0x95,
                 0xC9, 0xD9, 0x07, 0x75, 0x38, 0x47},
                {0x99, 0x69, 0xA2, 0x58, 0x6B, 0xF0, 0xA1, 0x72, 0x09, 0x0E,
                 0x6B, 0x6B, 0x10, 0x55, 0xDE, 0x80},
            },
            {
                {0x63, 0x3B, 0x74, 0xCB, 0xD5, 0xAA, 0xCB, 0x42, 0x50, 0x8B,
                 0xF1, 0xF3, 0xAF, 0xFF, 0x21, 0xB6},
                {0x2A, 0x5B, 0xB9, 0xB8, 0x9E, 0xD4, 0x53, 0x3D, 0x97, 0x2F,
                 0xB5, 0xF5, 0xC6, 0x16, 0x6A, 0x28},
                {0x64, 0xD8, 0x3E, 0xC8, 0xE9, 0xDF, 0xF0, 0x5E, 0xF6, 0xF4,
                 0x7D, 0x5A, 0xA5, 0x7C, 0xC6, 0x6E},
            },
        },
    };

    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/006-in.json"),
               SWICC_RET_SUCCESS);

    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    while (swicc_disk_tree_iter_next(&tree_iter, &tree) == SWICC_RET_SUCCESS)
    {
        static uint32_t const tree_file_count =
            sizeof(file_rcrd_id[0U]) / sizeof(file_rcrd_id[0U][0U]);
        static uint32_t const rcrd_count =
            sizeof(rcrd_data[0U][0U]) / sizeof(rcrd_data[0U][0U][0U]);
        static uint32_t const rcrd_len =
            sizeof(rcrd_data[0U][0U][0U]) / sizeof(rcrd_data[0U][0U][0U][0U]);

        for (uint8_t file_idx = 0U; file_idx < tree_file_count; ++file_idx)
        {
            swicc_fs_file_st file;
            swicc_ret_et const ret_file_lookup = swicc_disk_lutid_lookup(
                &disk, &tree, file_rcrd_id[tree_iter.tree_idx][file_idx],
                &file);
            CHECK_EQ(ret_file_lookup, SWICC_RET_SUCCESS);

            /* Only check contents of file when file lookup is successful. */
            if (ret_file_lookup == SWICC_RET_SUCCESS)
            {
                uint8_t *buf;
                uint8_t len;

                for (swicc_fs_rcrd_idx_kt rcrd_idx = 0U; rcrd_idx < rcrd_count;
                     ++rcrd_idx)
                {
                    swicc_ret_et const ret_file_rcrd =
                        swicc_disk_file_rcrd(tree, &file, rcrd_idx, &buf, &len);
                    CHECK_EQ(ret_file_rcrd, SWICC_RET_SUCCESS);

                    /* Only check contents when the record was found. */
                    if (ret_file_rcrd == SWICC_RET_SUCCESS)
                    {
                        bool const buf_len_equal = len == rcrd_len;
                        CHECK_TRUE(buf_len_equal);

                        /**
                         * Make sure the claimed buffer size is equal to the
                         * real one to avoid any unsafe memory accesses.
                         */
                        if (buf_len_equal)
                        {
                            CHECK_BUF_EQ(buf,
                                         rcrd_data[tree_iter.tree_idx][file_idx]
                                                  [rcrd_idx],
                                         len);
                        }
                        else
                        {
                            tauColouredPrintf(TAU_COLOUR_BRIGHTBLUE_,
                                              "[   INFO   ] ");
                            tauColouredPrintf(
                                TAU_COLOUR_DEFAULT_,
                                "Expected buffer length to be %u, got %u.\n",
                                rcrd_len, len);
                        }
                    }
                    else
                    {
                        tauColouredPrintf(TAU_COLOUR_BRIGHTBLUE_,
                                          "[   INFO   ] ");
                        tauColouredPrintf(
                            TAU_COLOUR_DEFAULT_,
                            "Failed to find record %u in file with ID 0x%04X "
                            "in tree %u which is unexpected.\n",
                            rcrd_idx,
                            file_rcrd_id[tree_iter.tree_idx][file_idx],
                            tree_iter.tree_idx);
                    }
                }
            }
        }
    }
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_file_rcrd_cnt__param_check)
{
    swicc_disk_tree_st *const tree = (swicc_disk_tree_st *)1U;
    swicc_fs_file_st *const file = (swicc_fs_file_st *)1U;
    uint32_t *const rcrd_cnt = (uint32_t *)1U;
    CHECK_EQ(swicc_disk_file_rcrd_cnt(NULL, file, rcrd_cnt),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_rcrd_cnt(tree, NULL, rcrd_cnt),
             SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_file_rcrd_cnt(tree, file, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_file_rcrd_cnt__disk)
{
    /* Tree -> File */
    static swicc_fs_id_kt file_rcrd_id[4U][2U] = {
        {0xE99D, 0x5ABD},
        {0x5240, 0xB1E9},
        {0x89E7, 0xC24A},
        {0x8EB4, 0xAF09},
    };

    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/006-in.json"),
               SWICC_RET_SUCCESS);

    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    while (swicc_disk_tree_iter_next(&tree_iter, &tree) == SWICC_RET_SUCCESS)
    {
        for (uint32_t file_idx = 0U;
             file_idx < sizeof(file_rcrd_id[0U]) / sizeof(file_rcrd_id[0U][0U]);
             ++file_idx)
        {
            swicc_fs_file_st file;
            swicc_ret_et const ret_file_lookup = swicc_disk_lutid_lookup(
                &disk, &tree, file_rcrd_id[tree_iter.tree_idx][file_idx],
                &file);
            CHECK_EQ(ret_file_lookup, SWICC_RET_SUCCESS);

            if (ret_file_lookup == SWICC_RET_SUCCESS)
            {
                uint32_t rcrd_cnt = 0U;
                CHECK_EQ(swicc_disk_file_rcrd_cnt(tree, &file, &rcrd_cnt),
                         SWICC_RET_SUCCESS);
                CHECK_EQ(rcrd_cnt, 3U);
            }
        }
    }
    swicc_disk_unload(&disk);
}

TEST(fs_disk, swicc_disk_tree_file_root__param_check)
{
    swicc_disk_tree_st *const tree = (swicc_disk_tree_st *)1U;
    swicc_fs_file_st *const file_root = (swicc_fs_file_st *)1U;
    CHECK_EQ(swicc_disk_tree_file_root(NULL, file_root), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_disk_tree_file_root(tree, NULL), SWICC_RET_PARAM_BAD);
}

TEST(fs_disk, swicc_disk_tree_file_root__disk)
{
    /* Tree -> File */
    static swicc_fs_id_kt const file_root_id[4U] = {0xE7C7, 0x1217, 0x40B1,
                                                    0xC3C0};

    swicc_disk_st disk = {0U};
    REQUIRE_EQ(swicc_diskjs_disk_create(&disk, "test/data/disk/006-in.json"),
               SWICC_RET_SUCCESS);

    swicc_disk_tree_iter_st tree_iter;
    CHECK_EQ(swicc_disk_tree_iter(&disk, &tree_iter), SWICC_RET_SUCCESS);
    swicc_disk_tree_st *tree;
    while (swicc_disk_tree_iter_next(&tree_iter, &tree) == SWICC_RET_SUCCESS)
    {
        swicc_fs_file_st file_root;
        CHECK_EQ(swicc_disk_tree_file_root(tree, &file_root),
                 SWICC_RET_SUCCESS);
        CHECK_EQ(file_root.hdr_file.id, file_root_id[tree_iter.tree_idx]);
    }
    swicc_disk_unload(&disk);
}

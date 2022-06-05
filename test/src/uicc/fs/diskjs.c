#include <tau/tau.h>

#include <cJSON.h>
#include <uicc/uicc.h>

/* No other choice for testing static functions... */
#include <uicc/src/fs/diskjs.c>

#define TEST_DATA_FOREACH(path_prefix, code)                                   \
    do                                                                         \
    {                                                                          \
        uint32_t path_prefix_len = strlen(path_prefix);                        \
        if (path_prefix_len > 128U)                                            \
        {                                                                      \
            WARN("Path prefix is too long.");                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            uint32_t file_idx = 0U;                                            \
            FILE *file_in;                                                     \
            FILE *file_out;                                                    \
                                                                               \
            char path_in[128U] = {0U};                                         \
            char path_out[128U] = {0U};                                        \
            memcpy(path_in, path_prefix, path_prefix_len);                     \
            memcpy(path_out, path_prefix, path_prefix_len);                    \
                                                                               \
            uint32_t const buf_size = 16384U;                                  \
            uint8_t *buf_in = malloc(buf_size);                                \
            uint8_t *buf_out = malloc(buf_size);                               \
            uint32_t buf_in_len = 0U;                                          \
            uint32_t buf_out_len = 0U;                                         \
                                                                               \
            for (;;)                                                           \
            {                                                                  \
                if (buf_in == NULL || buf_out == NULL)                         \
                {                                                              \
                    break;                                                     \
                }                                                              \
                bool failed = true;                                            \
                if (snprintf(&path_in[path_prefix_len],                        \
                             128U - path_prefix_len, "%03u-in.json",           \
                             file_idx) == 11U &&                               \
                    snprintf(&path_out[path_prefix_len],                       \
                             128U - path_prefix_len, "%03u-out.bin",           \
                             file_idx) == 11U)                                 \
                {                                                              \
                    if ((file_in = fopen(path_in, "r")))                       \
                    {                                                          \
                        if ((file_out = fopen(path_out, "r")))                 \
                        {                                                      \
                            if (fseek(file_in, 0U, SEEK_END) == 0 &&           \
                                fseek(file_out, 0U, SEEK_END) == 0)            \
                            {                                                  \
                                tauColouredPrintf(TAU_COLOUR_BRIGHTBLUE_,      \
                                                  "[   INFO   ] ");            \
                                tauColouredPrintf(                             \
                                    TAU_COLOUR_DEFAULT_,                       \
                                    "Data used: %s + (%03u-in.json, "          \
                                    "%03u-out.bin)\n",                         \
                                    path_prefix, file_idx, file_idx);          \
                                int64_t const file_in_size = ftell(file_in);   \
                                int64_t const file_out_size = ftell(file_out); \
                                if (file_in_size >= 0 && file_out_size >= 0 && \
                                    file_in_size <= buf_size &&                \
                                    file_out_size <= buf_size)                 \
                                {                                              \
                                    if (fseek(file_in, 0U, SEEK_SET) == 0 &&   \
                                        fseek(file_out, 0U, SEEK_SET) == 0)    \
                                    {                                          \
                                        if (fread(buf_in,                      \
                                                  (uint32_t)file_in_size, 1U,  \
                                                  file_in) ==                  \
                                                (file_in_size == 0 ? 0U        \
                                                                   : 1U) &&    \
                                            fread(buf_out,                     \
                                                  (uint32_t)file_out_size, 1U, \
                                                  file_out) ==                 \
                                                (file_out_size == 0 ? 0U       \
                                                                    : 1U))     \
                                        {                                      \
                                            buf_in_len =                       \
                                                (uint32_t)file_in_size;        \
                                            buf_out_len =                      \
                                                (uint32_t)file_out_size;       \
                                            failed = false;                    \
                                            file_idx++;                        \
                                            {code};                            \
                                        }                                      \
                                    }                                          \
                                }                                              \
                                else                                           \
                                {                                              \
                                    WARN("File size is invalid or too large "  \
                                         "to fit in pre-allocated buffers.");  \
                                }                                              \
                            }                                                  \
                            fclose(file_out);                                  \
                        }                                                      \
                        fclose(file_in);                                       \
                    }                                                          \
                }                                                              \
                if (failed)                                                    \
                {                                                              \
                    break;                                                     \
                }                                                              \
            }                                                                  \
            if (file_idx == 0U)                                                \
            {                                                                  \
                WARN("No files were tested.");                                 \
            }                                                                  \
            free(buf_in);                                                      \
            free(buf_out);                                                     \
        }                                                                      \
    } while (0U)

#define JSITEM_PRS_FT__PARAM_CHECK(func)                                       \
    do                                                                         \
    {                                                                          \
        cJSON const item_json;                                                 \
        uint8_t buf;                                                           \
        uint32_t buf_len = sizeof(buf);                                        \
        CHECK_EQ(func(NULL, 0U, &buf, &buf_len), UICC_RET_PARAM_BAD);          \
        CHECK_EQ(func(&item_json, 0U, NULL, &buf_len), UICC_RET_PARAM_BAD);    \
        CHECK_EQ(func(&item_json, 0U, &buf, NULL), UICC_RET_PARAM_BAD);        \
    } while (0)

#define JSITEM_PRS_FT__DATA(func, path_prefix, offset_prel)                    \
    do                                                                         \
    {                                                                          \
        static uint32_t const buf_prsd_size = 2048U;                           \
        uint8_t buf_prsd[buf_prsd_size];                                       \
        uint32_t buf_prsd_len = 0U;                                            \
        TEST_DATA_FOREACH(path_prefix, {                                       \
            cJSON *const obj =                                                 \
                cJSON_ParseWithLength((char *)buf_in, buf_in_len);             \
            if (obj == NULL)                                                   \
            {                                                                  \
                failed = true;                                                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                buf_prsd_len = buf_out_len;                                    \
                CHECK_EQ(func(obj, offset_prel, buf_prsd, &buf_prsd_len),      \
                         UICC_RET_SUCCESS);                                    \
                /**                                                            \
                 * @note This weird casting here is to avoid triggering        \
                 * -Wconversion.                                               \
                 */                                                            \
                int32_t const buf_prsd_len_after = (int32_t)buf_prsd_len;      \
                CHECK_BUF_EQ(buf_prsd, buf_out, (size_t)buf_prsd_len_after);   \
                cJSON_Delete(obj);                                             \
            }                                                                  \
        });                                                                    \
    } while (0)

static cJSON *itemjs_create(cJSON *const item_json, bool const add_type,
                            char const *const type)
{
    cJSON *base = item_json;
    if (item_json == NULL)
    {
        base = cJSON_CreateObject();
    }
    if (base == NULL)
    {
        WARN("Failed to create an item JSON object.");
    }
    else
    {
        do
        {
            if (add_type)
            {
                cJSON *const obj = cJSON_AddStringToObject(base, "type", type);
                if (obj == NULL)
                {
                    WARN("Failed to create and add a type object.");
                    break;
                }
            }
            return base;
        } while (0U);
        if (item_json == NULL)
        {
            cJSON_Delete(base);
        }
    }
    return NULL;
}

static cJSON *filejs_create(cJSON *const item_json, bool const add_id,
                            char const *const id, bool const add_sid,
                            char const *const sid, bool const add_contents,
                            char const *const contents)
{
    cJSON *base = item_json;
    if (item_json == NULL)
    {
        base = cJSON_CreateObject();
    }
    if (base == NULL)
    {
        WARN("Failed to create a file JSON object.");
    }
    else
    {
        do
        {
            if (add_id)
            {
                cJSON *const obj = cJSON_AddStringToObject(base, "id", id);
                if (obj == NULL)
                {
                    WARN("Failed to create and add an ID object.");
                    break;
                }
            }
            if (add_sid)
            {
                cJSON *const obj = cJSON_AddStringToObject(base, "sid", sid);
                if (obj == NULL)
                {
                    WARN("Failed to create and add an SID object.");
                    break;
                }
            }
            if (add_contents)
            {
                cJSON *const obj =
                    cJSON_AddStringToObject(base, "contents", contents);
                if (obj == NULL)
                {
                    WARN("Failed to create and add a contents object.");
                    break;
                }
            }
            return base;
        } while (0U);
        if (item_json == NULL)
        {
            cJSON_Delete(base);
        }
    }
    return NULL;
}

TEST(fs_diskjs, jsitem_prs_file_raw__param_check)
{
    cJSON item_json = {0U};
    uicc_fs_file_raw_st file = {0U};
    CHECK_EQ(jsitem_prs_file_raw(NULL, &file, 0U), UICC_RET_PARAM_BAD);
    CHECK_EQ(jsitem_prs_file_raw(&item_json, NULL, 0U), UICC_RET_PARAM_BAD);
}

TEST(fs_diskjs, jsitem_prs_file_raw__file_empty)
{
    cJSON *const file =
        filejs_create(NULL, false, NULL, false, NULL, false, NULL);
    if (file == NULL)
    {
        WARN("Failed to create a file object.");
    }
    else
    {
        uicc_fs_file_raw_st file_act = {0};
        /* Parsing empty JSON object should succeed as no field is mandatory. */
        uicc_ret_et const ret_empty = jsitem_prs_file_raw(file, &file_act, 0U);
        CHECK_EQ(ret_empty, UICC_RET_SUCCESS);
        cJSON_Delete(file);
    }
}

TEST(fs_diskjs, jsitem_prs_file_raw__file_w_id)
{
    char const id_str[] = "0CDE";
    uicc_fs_id_kt const id = 0x0CDE;
    uint32_t const offset_prel = 0x8C95797C;
    cJSON *const file =
        filejs_create(NULL, true, id_str, false, NULL, false, NULL);
    if (file == NULL)
    {
        WARN("Failed to create a file object.");
    }
    else
    {
        uicc_fs_file_raw_st file_act = {0};
        uicc_fs_file_raw_st file_exp = {
            .hdr_item = {0U},
            .hdr_item.offset_prel = offset_prel,
            .hdr_file =
                {
                    .id = id,
                    .sid = UICC_FS_SID_MISSING,
                },
        };

        uicc_ret_et const ret =
            jsitem_prs_file_raw(file, &file_act, offset_prel);
        CHECK_EQ(ret, UICC_RET_SUCCESS);
        CHECK_BUF_EQ(&file_act, &file_exp, sizeof(uicc_fs_file_raw_st));
        cJSON_Delete(file);
    }
}

TEST(fs_diskjs, jsitem_prs_file_raw__file_w_sid)
{
    char const sid_str[] = "C9";
    uicc_fs_sid_kt const sid = 0xC9;
    uint32_t const offset_prel = 0xE8E9A0BE;
    cJSON *const file =
        filejs_create(NULL, false, NULL, true, sid_str, false, NULL);
    if (file == NULL)
    {
        WARN("Failed to create a file object.");
    }
    else
    {
        uicc_fs_file_raw_st file_act = {0};
        uicc_fs_file_raw_st file_exp = {
            .hdr_item = {0U},
            .hdr_item.offset_prel = offset_prel,
            .hdr_file =
                {
                    .id = UICC_FS_ID_MISSING,
                    .sid = sid,
                },
        };

        uicc_ret_et const ret =
            jsitem_prs_file_raw(file, &file_act, offset_prel);
        CHECK_EQ(ret, UICC_RET_SUCCESS);
        CHECK_BUF_EQ(&file_act, &file_exp, sizeof(uicc_fs_file_raw_st));
        cJSON_Delete(file);
    }
}

TEST(fs_diskjs, jsitem_prs_file_raw__file_w_id_sid)
{
    char const id_str[] = "3F12";
    char const sid_str[] = "1F";
    uicc_fs_id_kt const id = 0x3F12;
    uicc_fs_sid_kt const sid = 0x1F;
    uint32_t const offset_prel = 0x27DC3505;
    cJSON *const file =
        filejs_create(NULL, true, id_str, true, sid_str, false, NULL);
    if (file == NULL)
    {
        WARN("Failed to create a file object.");
    }
    else
    {
        uicc_fs_file_raw_st file_act = {0};
        uicc_fs_file_raw_st file_exp = {
            .hdr_item = {0U},
            .hdr_item.offset_prel = offset_prel,
            .hdr_file =
                {
                    .id = id,
                    .sid = sid,
                },
        };

        uicc_ret_et const ret =
            jsitem_prs_file_raw(file, &file_act, offset_prel);
        CHECK_EQ(ret, UICC_RET_SUCCESS);
        CHECK_BUF_EQ(&file_act, &file_exp, sizeof(uicc_fs_file_raw_st));
        cJSON_Delete(file);
    }
}

TEST(fs_diskjs, jsitem_prs_type_str__param_check)
{
    char const type_str[] = "DIGN2";
    uicc_fs_item_type_et type;
    CHECK_EQ(jsitem_prs_type_str(NULL, 0U, &type), UICC_RET_PARAM_BAD);
    CHECK_EQ(jsitem_prs_type_str(type_str, sizeof(type_str) - 1U, NULL),
             UICC_RET_PARAM_BAD);
}

TEST(fs_diskjs, jsitem_prs_type_str__invalid)
{
    char const *const type_str[] = {
        "", "a", "q7lB7", "file_mff", "hexx", "aascii", " hex ",
    };
    for (uint8_t type_str_idx = 0U;
         type_str_idx < sizeof(type_str) / sizeof(type_str[0U]); ++type_str_idx)
    {
        uicc_fs_item_type_et type;
        CHECK_EQ(jsitem_prs_type_str(type_str[type_str_idx],
                                     (uint16_t)strlen(type_str[type_str_idx]),
                                     &type),
                 UICC_RET_SUCCESS);
        CHECK_EQ(type, UICC_FS_ITEM_TYPE_INVALID);
    }
}

TEST(fs_diskjs, jsitem_prs_type_str__valid)
{
    for (uint8_t type_idx = 0U; type_idx < ITEM_TYPE_COUNT; ++type_idx)
    {
        uicc_fs_item_type_et type_act;
        CHECK_EQ(jsitem_prs_type_str(item_type_str[type_idx],
                                     (uint16_t)strlen(item_type_str[type_idx]),
                                     &type_act),
                 UICC_RET_SUCCESS);
        CHECK_EQ(type_act, item_type_enum[type_idx]);
    }
}

TEST(fs_diskjs, jsitem_prs_demux__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_demux);
}

TEST(fs_diskjs, jsitem_prs_demux__item_empty)
{
    cJSON *const item_json = itemjs_create(NULL, false, NULL);
    uint8_t buf;
    uint32_t buf_len = sizeof(buf);
    CHECK_EQ(jsitem_prs_demux(item_json, 0U, &buf, &buf_len), UICC_RET_ERROR);
    cJSON_Delete(item_json);
}

/**
 * @warning There is no for 'jsitem_prs_demux' test which parses the item.
 * That's because this function only demuxes so the parsing will be tested
 * elsewhere.
 */

TEST(fs_diskjs, jsitem_prs_file_folder__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_folder);
}

TEST(fs_diskjs, jsitem_prs_file_folder__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_folder, "test/data/folder/",
                        0x0000001D);
}

TEST(fs_diskjs, jsitem_prs_file_mf__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_mf);
}

TEST(fs_diskjs, jsitem_prs_file_mf__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_mf, "test/data/file_mf/", 0x361A49BB);
}

TEST(fs_diskjs, jsitem_prs_file_adf__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_adf);
}

TEST(fs_diskjs, jsitem_prs_file_adf__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_adf, "test/data/file_adf/", 0x73BDD9C1);
}

TEST(fs_diskjs, jsitem_prs_file_df__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_df);
}

TEST(fs_diskjs, jsitem_prs_file_df__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_df, "test/data/file_df/", 0xB8244A98);
}

TEST(fs_diskjs, jsitem_prs_file_ef_transparent__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_ef_transparent);
}

TEST(fs_diskjs, jsitem_prs_file_ef_transparent__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_ef_transparent,
                        "test/data/file_ef_transparent/", 0x3F65CAC6);
}

TEST(fs_diskjs, jsitem_prs_file_ef_linearfixed__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_ef_linearfixed);
}

TEST(fs_diskjs, jsitem_prs_file_ef_linearfixed__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_ef_linearfixed,
                        "test/data/file_ef_linearfixed/", 0xF3A5A9A6);
}

TEST(fs_diskjs, jsitem_prs_file_ef_cyclic__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_file_ef_cyclic);
}

TEST(fs_diskjs, jsitem_prs_file_ef_cyclic__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_file_ef_cyclic, "test/data/file_ef_cyclic/",
                        0x16FE37F4);
}

TEST(fs_diskjs, jsitem_prs_item_dato_bertlv__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_item_dato_bertlv);
}

TEST(fs_diskjs, jsitem_prs_item_dato_bertlv__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_item_dato_bertlv,
                        "test/data/item_dato_bertlv/", 0xBA2A382C);
}

TEST(fs_diskjs, jsitem_prs_item_hex__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_item_hex);
}

TEST(fs_diskjs, jsitem_prs_item_hex__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_item_hex, "test/data/item_hex/", 0x07AE2D22);
}

TEST(fs_diskjs, jsitem_prs_item_ascii__param_check)
{
    JSITEM_PRS_FT__PARAM_CHECK(jsitem_prs_item_ascii);
}

TEST(fs_diskjs, jsitem_prs_item_ascii__data)
{
    JSITEM_PRS_FT__DATA(jsitem_prs_item_ascii, "test/data/item_ascii/",
                        0x3EA78A0C);
}

TEST(fs_diskjs, disk_json_prs__param_check)
{
    uicc_disk_st disk;
    cJSON const disk_json;
    CHECK_EQ(disk_json_prs(NULL, &disk_json), UICC_RET_PARAM_BAD);
    CHECK_EQ(disk_json_prs(&disk, NULL), UICC_RET_PARAM_BAD);
}

TEST(fs_diskjs, disk_json_prs__data)
{
    uicc_disk_st disk = {};
    TEST_DATA_FOREACH("test/data/disk/", {
        cJSON *const obj = cJSON_ParseWithLength((char *)buf_in, buf_in_len);
        if (obj == NULL)
        {
            failed = true;
        }
        else
        {
            uicc_ret_et const ret_disk_prs = disk_json_prs(&disk, obj);
            CHECK_EQ(ret_disk_prs, UICC_RET_SUCCESS);
            (void)buf_out_len;
            (void)buf_out;

            uint32_t buf_out_idx = 0U;
            uicc_disk_tree_st *tree = disk.root;
            while (tree != NULL)
            {
                CHECK_LE(tree->len, buf_out_len - buf_out_idx);
                if (buf_out_idx + tree->len >= buf_out_len)
                {
                    break;
                }
                /**
                 * @note This weird casting here is to avoid triggering
                 * -Wconversion.
                 */
                int32_t const buf_len = (int32_t)tree->len;
                CHECK_BUF_EQ(tree->buf, &buf_out[buf_out_idx], (size_t)buf_len);
                buf_out_idx += tree->len;
                tree = tree->next;
            }

            if (buf_out_idx + sizeof(disk.lutid.count) > buf_out_len)
            {
                WARN("Test out file is too short to contain ID LUT item "
                     "count (4B).");
                CHECK_LE(buf_out_idx + sizeof(disk.lutid.count), buf_out_len);
            }
            else
            {
                /**
                 * The test out file contains not only expected contents but
                 * some extra data that we extract here. Safe cast since this is
                 * checked to be safe in the 'if' condition.
                 */
                uint32_t const lutid_count_exp =
                    *(uint32_t *)&buf_out[buf_out_idx];
                /* Safe case due to 'if' condition. */
                buf_out_idx = (uint32_t)(buf_out_idx + sizeof(lutid_count_exp));

                /* Make sure the LUTs were also created correctly. */
                CHECK_NE((void *)disk.lutid.buf1, NULL);
                CHECK_NE((void *)disk.lutid.buf2, NULL);
                CHECK_EQ(disk.lutid.count, lutid_count_exp);
                CHECK_EQ(disk.lutid.size_item1, sizeof(uicc_fs_id_kt));
                CHECK_EQ(disk.lutid.size_item2,
                         sizeof(uint32_t) + sizeof(uint8_t));
                if (buf_out_idx + lutid_count_exp * (disk.lutid.size_item1 +
                                                     disk.lutid.size_item2) >
                    buf_out_len)
                {
                    WARN("Test out file is too short to contain ID LUT "
                         "entries.");
                    CHECK_LE(buf_out_idx +
                                 lutid_count_exp * (disk.lutid.size_item1 +
                                                    disk.lutid.size_item2),
                             buf_out_len);
                }
                else
                {
                    /**
                     * @note This weird casting here is to avoid triggering
                     * -Wconversion. These are UNSAFE casts.
                     */
                    int32_t const lutid_buf1_len =
                        (int32_t)(lutid_count_exp * disk.lutid.size_item1);
                    int32_t const lutid_buf2_len =
                        (int32_t)(lutid_count_exp * disk.lutid.size_item2);
                    CHECK_BUF_EQ(disk.lutid.buf1, &buf_out[buf_out_idx],
                                 (size_t)lutid_buf1_len);
                    buf_out_idx =
                        (uint32_t)(buf_out_idx + (uint32_t)lutid_buf1_len);
                    CHECK_BUF_EQ(disk.lutid.buf2, &buf_out[buf_out_idx],
                                 (size_t)lutid_buf2_len);
                    buf_out_idx =
                        (uint32_t)(buf_out_idx + (uint32_t)lutid_buf2_len);
                }
            }

            if (ret_disk_prs == UICC_RET_SUCCESS)
            {
                uicc_disk_unload(&disk);
            }
            cJSON_Delete(obj);
        }
    });
}

#include "uicc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
static char const *const item_type_str[] = {
    [UICC_FS_ITEM_TYPE_INVALID] = "INV",
    [UICC_FS_ITEM_TYPE_FILE_MF] = "MF",
    [UICC_FS_ITEM_TYPE_FILE_ADF] = "ADF",
    [UICC_FS_ITEM_TYPE_FILE_DF] = "DF",
    [UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT] = "EF Transparent",
    [UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED] = "EF Linear-Fixed",
    [UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC] = "EF Cyclic",
    [UICC_FS_ITEM_TYPE_DATO_BERTLV] = "DO BER-TLV",
    [UICC_FS_ITEM_TYPE_HEX] = "HEX",
    [UICC_FS_ITEM_TYPE_ASCII] = "ASCII",
};

static uicc_ret_et fs_item_str(char *const buf_str, uint16_t *const buf_str_len,
                               uicc_disk_tree_st *const tree,
                               uicc_fs_item_hdr_st const *const item,
                               uint8_t const depth)
{
    uicc_ret_et ret = UICC_RET_ERROR;

    /**
     * Depth string i.e. a string to create a left margin that simulates nesting
     * of folders and files.
     */
    char ds[(depth * 4U) + 1U];
    memset(ds, ' ', (depth * 4U));
    ds[(depth * 4U)] = '\0';

    uint16_t const buf_size = *buf_str_len;
    *buf_str_len = 0;

    uint16_t buf_unused_len = buf_size;
    int32_t dbg_str_len_tmp = snprintf(
        buf_str, buf_unused_len,
        // clang-format off
        "\n%s(" CLR_KND("%s") ""
        "\n%s  (" CLR_KND("Size") " " CLR_VAL("%u") ")"
        "\n%s  (" CLR_KND("LCS") " " CLR_VAL("%u") ")"
        "\n%s  (" CLR_KND("Type") " " CLR_VAL("%u") ")",
        // clang-format on
        ds, item_type_str[item->type], ds, item->size, ds, item->lcs, ds,
        item->type);
    if (dbg_str_len_tmp < 0 || (int64_t)buf_unused_len - dbg_str_len_tmp <= 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        /* Safe because it was checked to not become negative. */
        buf_unused_len = (uint16_t)(buf_unused_len - dbg_str_len_tmp);
    }

    /* Create the common file header. */
    switch (item->type)
    {
    case UICC_FS_ITEM_TYPE_FILE_MF:
    case UICC_FS_ITEM_TYPE_FILE_ADF:
    case UICC_FS_ITEM_TYPE_FILE_DF:
    case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
    case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
    case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC: {
        uicc_fs_file_hdr_st file;
        if (uicc_fs_file_hdr_prs(
                (uicc_fs_file_hdr_raw_st *)&tree->buf[item->offset_trel],
                &file) != UICC_RET_SUCCESS)
        {
            return ret;
        }
        dbg_str_len_tmp = snprintf(
            &buf_str[buf_size - buf_unused_len], buf_unused_len,
            // clang-format off
            "\n%s  (" CLR_KND("ID") " " CLR_VAL("0x%04X") ")"
            "\n%s  (" CLR_KND("SID") " " CLR_VAL("0x%02X") ")"
            "\n%s  (" CLR_KND("Name") " " CLR_VAL("'%s'") ")",
            // clang-format on
            ds, file.id, ds, file.sid, ds, file.name);
        if (dbg_str_len_tmp < 0 ||
            (int64_t)buf_unused_len - dbg_str_len_tmp <= 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        else
        {
            /* Safe because it was checked to not become negative. */
            buf_unused_len = (uint16_t)(buf_unused_len - dbg_str_len_tmp);
        }
        break;
    }
    default:
        break;
    }

    /* For EFs with records, also add the record length to the header. */
    {
        if (item->type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED)
        {
            uicc_fs_ef_linearfixed_hdr_raw_st const *const ef_linearfixed =
                (uicc_fs_ef_linearfixed_hdr_raw_st *)&tree
                    ->buf[item->offset_trel];
            dbg_str_len_tmp =
                snprintf(&buf_str[buf_size - buf_unused_len], buf_unused_len,
                         // clang-format off
                     "\n%s  (" CLR_KND("Record Length") " " CLR_VAL("%u") ")"
                     "\n%s  (" CLR_KND("Contents"),
                         // clang-format on
                         ds, ef_linearfixed->rcrd_size, ds);
        }
        else if (item->type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
        {
            uicc_fs_ef_cyclic_hdr_raw_st const *const ef_cyclic =
                (uicc_fs_ef_cyclic_hdr_raw_st *)&tree->buf[item->offset_trel];
            dbg_str_len_tmp =
                snprintf(&buf_str[buf_size - buf_unused_len], buf_unused_len,
                         // clang-format off
                     "\n%s  (" CLR_KND("Record Length") " " CLR_VAL("%u") ")"
                     "\n%s  (" CLR_KND("Contents"),
                         // clang-format on
                         ds, ef_cyclic->rcrd_size, ds);
        }
        else
        {
            dbg_str_len_tmp =
                snprintf(&buf_str[buf_size - buf_unused_len], buf_unused_len,
                         "\n%s  (" CLR_KND("Contents"), ds);
        }
        if (dbg_str_len_tmp < 0 ||
            (int64_t)buf_unused_len - dbg_str_len_tmp <= 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        else
        {
            /* Safe because it was checked to not become negative. */
            buf_unused_len = (uint16_t)(buf_unused_len - dbg_str_len_tmp);
        }
    }

    switch (item->type)
    {
    case UICC_FS_ITEM_TYPE_FILE_MF:
    case UICC_FS_ITEM_TYPE_FILE_ADF:
    case UICC_FS_ITEM_TYPE_FILE_DF: {
        /* Success by default because while loop may never run. */
        uicc_ret_et ret_item = UICC_RET_SUCCESS;

        uint32_t const hdr_len = item->type == UICC_FS_ITEM_TYPE_FILE_ADF
                                     ? sizeof(uicc_fs_adf_hdr_raw_st)
                                     : sizeof(uicc_fs_file_hdr_raw_st);

        if (item->offset_trel + hdr_len > UINT32_MAX)
        {
            return ret;
        }

        /* Safe cast due to check against uint32 max. */
        uint32_t const data_offset_start =
            (uint32_t)(item->offset_trel + hdr_len);
        /* Safe cast since size is at least as large as the header. */
        uint32_t const data_len = (uint32_t)(item->size - hdr_len);

        uint32_t data_idx = 0U;
        while (data_idx < data_len)
        {
            uicc_fs_item_hdr_raw_st *const data_item_hdr =
                (uicc_fs_item_hdr_raw_st *)&tree
                    ->buf[data_offset_start + data_idx];

            uicc_fs_item_hdr_st data_item;
            ret_item = uicc_fs_item_hdr_prs(data_item_hdr, &data_item);
            if (ret_item != UICC_RET_SUCCESS)
            {
                break;
            }
            data_item.offset_trel = data_offset_start + data_idx;

            /* Length the buffer length that is still unused. */
            uint16_t buf_str_len_nested = buf_unused_len;

            /**
             * Unsafe cast that relies on nesting that's less than 256 layers
             * deep.
             */
            ret_item = fs_item_str(&buf_str[buf_size - buf_unused_len],
                                   &buf_str_len_nested, tree, &data_item,
                                   (uint8_t)(depth + 1U));
            if (ret_item != UICC_RET_SUCCESS)
            {
                break;
            }

            if (buf_unused_len - buf_str_len_nested <= 0)
            {
                ret_item = UICC_RET_BUFFER_TOO_SHORT;
                break;
            }
            /* Safe case because checked that it won't become negative. */
            buf_unused_len = (uint16_t)(buf_unused_len - buf_str_len_nested);

            data_idx += data_item.size;
            ret_item = UICC_RET_SUCCESS;
        }
        ret = ret_item;
        break;
    }
    case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
    case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
    case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT: {
        uint8_t const ef_hdr_extra =
            item->type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||
                    item->type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC
                ? sizeof(uicc_fs_ef_linearfixed_hdr_raw_st) -
                      sizeof(uicc_fs_file_hdr_raw_st)
                : 0U;
        /* Safe cast since size is guaranteed to have at least a header. */
        uint32_t const data_len =
            (uint32_t)(item->size - sizeof(uicc_fs_file_hdr_raw_st) -
                       ef_hdr_extra);
        if (buf_unused_len - (int32_t)(data_len * 3) < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        for (uint32_t data_idx = 0U; data_idx < data_len; ++data_idx)
        {
            uint8_t const data_byte =
                tree->buf[ef_hdr_extra + item->offset_trel +
                          sizeof(uicc_fs_file_hdr_raw_st) + data_idx];
            dbg_str_len_tmp =
                snprintf(&buf_str[buf_size - buf_unused_len], buf_unused_len,
                         " " CLR_VAL("%02X"), data_byte);
            if (dbg_str_len_tmp < 0 ||
                (int64_t)buf_unused_len - dbg_str_len_tmp <= 0)
            {
                return UICC_RET_BUFFER_TOO_SHORT;
            }
            else
            {
                /* Safe because it was checked to not become negative. */
                buf_unused_len = (uint16_t)(buf_unused_len - dbg_str_len_tmp);
            }
        }
        ret = UICC_RET_SUCCESS;
        break;
    }
    case UICC_FS_ITEM_TYPE_DATO_BERTLV:
    case UICC_FS_ITEM_TYPE_ASCII:
    case UICC_FS_ITEM_TYPE_HEX:
    case UICC_FS_ITEM_TYPE_INVALID:
        /**
         * These types will never occur in the internal FS representation
         * because these are only used to construct a byte array.
         */
        ret = UICC_RET_SUCCESS;
        break;
    };

    switch (item->type)
    {
    case UICC_FS_ITEM_TYPE_FILE_MF:
    case UICC_FS_ITEM_TYPE_FILE_ADF:
    case UICC_FS_ITEM_TYPE_FILE_DF:
    case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
    case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
    case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
        /* End the contents and item expressions with two ')'. */
        if (buf_unused_len - 2 < 0)
        {
            return UICC_RET_BUFFER_TOO_SHORT;
        }
        buf_str[buf_size - (buf_unused_len - 0)] = ')';
        buf_str[buf_size - (buf_unused_len - 1)] = ')';
        /* Safe cast because checked to not be negative. */
        buf_unused_len = (uint16_t)(buf_unused_len - 2);
        break;
    default:
        break;
    }

    /* Safe cast since unused length is never greater than the buffer size. */
    *buf_str_len = (uint16_t)(buf_size - buf_unused_len);

    return ret;
}
#endif

uicc_ret_et uicc_dbg_disk_str(char *const buf_str, uint16_t *const buf_str_len,
                              uicc_disk_st const *const disk)
{
#ifdef DEBUG
    uint16_t const buf_size = *buf_str_len;
    *buf_str_len = 0U;
    uint16_t buf_unused_len = buf_size;

    int32_t disk_hdr_str_len =
        snprintf(buf_str, buf_unused_len,
                 "(" CLR_KND("Disk") "\n  (" CLR_KND("Contents") " ");
    if (disk_hdr_str_len < 0 || (int64_t)buf_unused_len - disk_hdr_str_len <= 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    else
    {
        /* Safe because it was checked to not become negative. */
        buf_unused_len = (uint16_t)(buf_unused_len - disk_hdr_str_len);
    }

    uicc_disk_tree_st *tree = disk->root;
    while (tree != NULL)
    {
        uint32_t disk_idx = 0U;
        while (disk_idx < tree->len)
        {
            uicc_fs_item_hdr_raw_st *const item_hdr =
                (uicc_fs_item_hdr_raw_st *)&tree->buf[disk_idx];

            uicc_fs_item_hdr_st item;
            if (uicc_fs_item_hdr_prs(item_hdr, &item) != UICC_RET_SUCCESS)
            {
                return UICC_RET_ERROR;
            }
            item.offset_trel = disk_idx;

            uint16_t dbg_str_len = buf_unused_len;
            uicc_ret_et ret_item =
                fs_item_str(&buf_str[buf_size - buf_unused_len], &dbg_str_len,
                            tree, &item, 1U);
            if (ret_item != UICC_RET_SUCCESS)
            {
                return ret_item;
            }

            if ((int64_t)buf_unused_len - dbg_str_len < 0)
            {
                return UICC_RET_BUFFER_TOO_SHORT;
            }
            /* Safe cast because was checked and won't become negative. */
            buf_unused_len = (uint16_t)(buf_unused_len - dbg_str_len);

            disk_idx += item.size;
        }
        tree = tree->next;
    }

    /* End the disk expression and add a null-terminator after the string. */
    if (buf_unused_len - 3 < 0)
    {
        return UICC_RET_BUFFER_TOO_SHORT;
    }
    buf_str[buf_size - (buf_unused_len - 0)] = ')';
    buf_str[buf_size - (buf_unused_len - 1)] = ')';
    buf_str[buf_size - (buf_unused_len - 2)] = '\0';

    /* Safe case because unused len is never larger than size. */
    *buf_str_len = (uint16_t)(buf_size - (buf_unused_len - 3));
    return UICC_RET_SUCCESS;
#else
    *buf_str_len = 0U;
    return UICC_RET_SUCCESS;
#endif
}

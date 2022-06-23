#include <cJSON.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <swicc/swicc.h>

/**
 * @warning The following 2 arrays (and the count variable) must be kept in sync
 * when edited. This is very important!
 */
#define ITEM_TYPE_COUNT 9U /* Excludes the 'invalid' type */
static char const *const item_type_str[ITEM_TYPE_COUNT] = {
    "file_mf",
    "file_adf",
    "file_df",
    "file_ef_transparent",
    "file_ef_linear-fixed",
    "file_ef_cyclic",
    "dato_ber-tlv",
    "hex",
    "ascii",
};
static swicc_fs_item_type_et const item_type_enum[ITEM_TYPE_COUNT] = {
    SWICC_FS_ITEM_TYPE_FILE_MF,
    SWICC_FS_ITEM_TYPE_FILE_ADF,
    SWICC_FS_ITEM_TYPE_FILE_DF,
    SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT,
    SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED,
    SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC,
    SWICC_FS_ITEM_TYPE_DATO_BERTLV,
    SWICC_FS_ITEM_TYPE_HEX,
    SWICC_FS_ITEM_TYPE_ASCII,
};
static_assert(sizeof(item_type_str) / sizeof(item_type_str[0U]) ==
                      sizeof(item_type_enum) / sizeof(item_type_enum[0U]) &&
                  sizeof(item_type_str) / sizeof(item_type_str[0U]) ==
                      ITEM_TYPE_COUNT,
              "Item type string array and item type enumeration member array "
              "length are not equal which may lead to errors in diskjs.");

/**
 * Used when creating a swICC FS disk. The 'start' size is the initial buffer
 * size, and if it's not large enough, it will grow by the 'resize' amount.
 */
#define DISK_SIZE_START 512U
#define DISK_SIZE_RESIZE 256U

/**
 * A function type for item type parsers i.e. parsers that parse a specific type
 * of item (encoded as a JSON object).
 */
typedef swicc_ret_et jsitem_prs_ft(cJSON const *const item_json,
                                   uint32_t const offset_prel,
                                   uint8_t *const buf, uint32_t *const buf_len);
/**
 * Declare early to avoid having to provide delcarations for all the individual
 * type parsers here.
 */
static jsitem_prs_ft *const jsitem_prs[];

/**
 * @brief Given a JSON item that contains a file (MF, ADF, DF, EF), parse it and
 * populate the raw file struct.
 * @param item_json
 * @param file_raw
 * @param offset_prel Offset from the first byte of the parent to the first byte
 * of the file header.
 * @return Return code.
 */
static swicc_ret_et jsitem_prs_file_raw(cJSON const *const item_json,
                                        swicc_fs_file_raw_st *const file_raw,
                                        uint32_t const offset_prel)
{
    if (item_json == NULL || file_raw == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    /**
     * @todo Check type of item passed to make sure it's a file type.
     */

    swicc_ret_et ret = SWICC_RET_ERROR;
    if (cJSON_IsObject(item_json) == true)
    {
        memset(file_raw, 0U, sizeof(*file_raw));

        swicc_ret_et ret_id = SWICC_RET_ERROR;
        cJSON *const id_obj = cJSON_GetObjectItemCaseSensitive(item_json, "id");
        if (id_obj != NULL && cJSON_IsString(id_obj) == true)
        {
            /* Has ID. */
            char *const id_str = cJSON_GetStringValue(id_obj);
            swicc_fs_id_kt id;
            if (id_str != NULL && strlen(id_str) == sizeof(id) * 2U)
            {
                uint32_t id_len = sizeof(id);
                if (swicc_hexstr_bytearr(id_str, sizeof(id) * 2U,
                                         (uint8_t *)&id,
                                         &id_len) == SWICC_RET_SUCCESS &&
                    id_len == sizeof(id))
                {
                    file_raw->hdr_file.id = be16toh(id);
                    ret_id = SWICC_RET_SUCCESS;
                }
                else
                {
                    printf("File: Failed to convert ID hex string to a byte "
                           "array.\n");
                }
            }
            else
            {
                printf(
                    "File: ID ('%s') length invalid (got %lu, expected 4).\n",
                    id_str, strlen(id_str));
            }
        }
        else
        {
            /* Has no ID. */
            file_raw->hdr_file.id = SWICC_FS_ID_MISSING;
            ret_id = SWICC_RET_SUCCESS;
        }

        swicc_ret_et ret_sid = SWICC_RET_ERROR;
        cJSON *const sid_obj =
            cJSON_GetObjectItemCaseSensitive(item_json, "sid");
        if (sid_obj != NULL && cJSON_IsString(sid_obj) == true)
        {
            /* Has SID. */
            char *const sid_str = cJSON_GetStringValue(sid_obj);
            swicc_fs_sid_kt sid;
            if (sid_str != NULL && strlen(sid_str) == sizeof(sid) * 2U)
            {
                uint32_t sid_len = sizeof(sid);
                if (swicc_hexstr_bytearr(sid_str, sizeof(sid) * 2U,
                                         (uint8_t *)&sid,
                                         &sid_len) == SWICC_RET_SUCCESS &&
                    sid_len == sizeof(sid))
                {
                    file_raw->hdr_file.sid = sid;
                    ret_sid = SWICC_RET_SUCCESS;
                }
                else
                {
                    printf("File: Failed to convert SID hex string to a byte "
                           "array.\n");
                }
            }
            else
            {
                printf(
                    "File: SID ('%s') length invalid (got %lu, expected 2).\n",
                    sid_str, strlen(sid_str));
            }
        }
        else
        {
            /* Has no SID. */
            file_raw->hdr_file.sid = SWICC_FS_SID_MISSING;
            ret_sid = SWICC_RET_SUCCESS;
        }

        if (ret_id == SWICC_RET_SUCCESS && ret_sid == SWICC_RET_SUCCESS)
        {
            file_raw->hdr_item.offset_prel = offset_prel;
            ret = SWICC_RET_SUCCESS;
        }
    }
    return ret;
}

/**
 * @brief Parse an item type string (as contained in the disk JSON file) to a
 * member for the item type enum.
 * @param type_str
 * @param type_str_len Length of the type string to not rely on 'strlen' in case
 * of a not null-terminated string.
 * @param type Where the type will be written. If none of the types match, this
 * will get the 'invalid' enum member.
 * @return Return code.
 */
static swicc_ret_et jsitem_prs_type_str(char const *const type_str,
                                        uint16_t const type_str_len,
                                        swicc_fs_item_type_et *const type)
{
    if (type_str == NULL || type == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    *type = SWICC_FS_ITEM_TYPE_INVALID;
    for (uint8_t type_idx = 0U; type_idx < ITEM_TYPE_COUNT; ++type_idx)
    {
        if (type_str_len == strlen(item_type_str[type_idx]) &&
            memcmp(type_str, item_type_str[type_idx],
                   strlen(item_type_str[type_idx])) == 0)
        {
            *type = item_type_enum[type_idx];
            break;
        }
    }
    return SWICC_RET_SUCCESS;
}

/**
 * @brief Parse an item in the JSON disk and write the parsed representation (in
 * swICC FS disk format) into the given buffer.
 * @param item Item to parse.
 * @param buf Where to write the parsed item.
 * @param buf_len Must hold the maximum size of the buffer. The size of the
 * parsed representation will be written here on success.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_demux;
static swicc_ret_et jsitem_prs_demux(cJSON const *const item_json,
                                     uint32_t const offset_prel,
                                     uint8_t *const buf,
                                     uint32_t *const buf_len)
{
    if (item_json == NULL || buf == NULL || buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    cJSON *const type_obj = cJSON_GetObjectItemCaseSensitive(item_json, "type");
    if (cJSON_IsObject(item_json) == true && type_obj != NULL &&
        cJSON_IsString(type_obj) == true)
    {
        char *const type_str = cJSON_GetStringValue(type_obj);
        if (type_str != NULL)
        {
            swicc_fs_item_type_et type;
            ret = jsitem_prs_type_str(type_str, (uint16_t)strlen(type_str),
                                      &type);
            if (ret == SWICC_RET_SUCCESS && type != SWICC_FS_ITEM_TYPE_INVALID)
            {
                ret = jsitem_prs[type](item_json, offset_prel, buf, buf_len);
                /* Basically returns the return code of prs. */
            }
        }
    }
    else
    {
        printf("Item: 'type' missing or not of type: string.\n");
    }
    return ret;
}

/**
 * @brief Parses the 'contents' attribute of folder items (MF, DF, ADF). This is
 * exactly the same for all folders so this is just implemented here once to
 * avoid duplicating code.
 * @param item_json The whole folder item (not the 'contents' attribute).
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 * @note Expects the JSON object to be a folder blindly.
 */
static jsitem_prs_ft jsitem_prs_file_folder;
static swicc_ret_et jsitem_prs_file_folder(cJSON const *const item_json,
                                           uint32_t const offset_prel,
                                           uint8_t *const buf,
                                           uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    /**
     * @todo Check type of item passed to make sure it's a folder type.
     */

    swicc_ret_et ret = SWICC_RET_ERROR;
    cJSON *const contents_obj =
        cJSON_GetObjectItemCaseSensitive(item_json, "contents");
    if (contents_obj != NULL && cJSON_IsNull(contents_obj) == true)
    {
        *buf_len = 0;
        ret = SWICC_RET_SUCCESS;
    }
    else if (contents_obj != NULL && cJSON_IsArray(contents_obj) == true)
    {
        cJSON *item;
        uint32_t items_len = 0U; /* Parsed length. */
        swicc_ret_et ret_item;
        cJSON_ArrayForEach(item, contents_obj)
        {
            uint32_t item_size = *buf_len - items_len;
            if (offset_prel + items_len > UINT32_MAX)
            {
                break;
            }
            /* Safe cast because this is checked to not overflow. */
            ret_item =
                jsitem_prs_demux(item, (uint32_t)(offset_prel + items_len),
                                 &buf[items_len], &item_size);
            if (ret_item != SWICC_RET_SUCCESS)
            {
                break;
            }
            /* Increase total items length by size of parsed item. */
            items_len += item_size;
        }

        if (ret_item == SWICC_RET_SUCCESS && *buf_len >= items_len)
        {
            *buf_len = items_len;
            ret = SWICC_RET_SUCCESS;
        }
        else
        {
            /**
             * Buffer is too short but the item return is not indicating this.
             */
            ret = ret_item;
            if (*buf_len < items_len && ret_item != SWICC_RET_BUFFER_TOO_SHORT)
            {
                /* Unexpected so can't recover. */
                ret = SWICC_RET_ERROR;
            }
        }
    }
    else
    {
        printf("Folder: 'contents' missing or not of type: null, array.\n");
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse it as a MF and write
 * the parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_file_mf;
static swicc_ret_et jsitem_prs_file_mf(cJSON const *const item_json,
                                       uint32_t const offset_prel,
                                       uint8_t *const buf,
                                       uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    /* Parse MF as a DF then change the item type after parsing. */
    swicc_ret_et ret = jsitem_prs[SWICC_FS_ITEM_TYPE_FILE_DF](
        item_json, offset_prel, buf, buf_len);
    if (ret == SWICC_RET_SUCCESS)
    {
        swicc_fs_file_raw_st *file_raw = (swicc_fs_file_raw_st *)buf;
        file_raw->hdr_item.type = SWICC_FS_ITEM_TYPE_FILE_MF;
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse it as a ADF and write
 * the parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_file_adf;
static swicc_ret_et jsitem_prs_file_adf(cJSON const *const item_json,
                                        uint32_t const offset_prel,
                                        uint8_t *const buf,
                                        uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    swicc_fs_file_raw_st *const file_raw = (swicc_fs_file_raw_st *)buf;
    swicc_fs_adf_hdr_raw_st *const adf_hdr_raw =
        (swicc_fs_adf_hdr_raw_st *)file_raw->data;

    uint32_t const hdr_size =
        swicc_fs_item_hdr_raw_size[SWICC_FS_ITEM_TYPE_FILE_ADF];

    cJSON *const aid_obj = cJSON_GetObjectItemCaseSensitive(item_json, "name");
    if (aid_obj != NULL && cJSON_IsObject(aid_obj) == true)
    {
        if (*buf_len < hdr_size)
        {
            ret = SWICC_RET_BUFFER_TOO_SHORT;
        }
        else
        {
            /* Excess bytes in the AID should be 0. */
            memset(&adf_hdr_raw->aid, 0U, sizeof(adf_hdr_raw->aid));

            uint32_t aid_len = sizeof(adf_hdr_raw->aid);
            ret = jsitem_prs_demux(aid_obj, 0U /* Not used. */,
                                   (uint8_t *)&adf_hdr_raw->aid, &aid_len);
            if (ret == SWICC_RET_SUCCESS)
            {
                /**
                 * PIX can be 0 bytes long so the AID must only contain an RID.
                 */
                if (aid_len < SWICC_FS_ADF_AID_RID_LEN)
                {
                    printf("File ADF: AID is too short to contain the RID (got "
                           "%u, expected >=%u).\n",
                           aid_len, SWICC_FS_ADF_AID_RID_LEN);
                    ret = SWICC_RET_ERROR;
                }
                else
                {

                    ret = jsitem_prs_file_raw(item_json, file_raw, offset_prel);
                    if (ret == SWICC_RET_SUCCESS)
                    {
                        /**
                         * Safe because buffer length is greater than
                         * header length.
                         */
                        uint32_t items_len = (uint32_t)(*buf_len - hdr_size);
                        ret = jsitem_prs_file_folder(
                            item_json, hdr_size, &buf[hdr_size], &items_len);
                        if (ret == SWICC_RET_SUCCESS)
                        {
                            file_raw->hdr_item.lcs = SWICC_FS_LCS_OPER_ACTIV;
                            file_raw->hdr_item.type =
                                SWICC_FS_ITEM_TYPE_FILE_ADF;
                            /**
                             * Safe cast because parsing would fail if there
                             * was not enough buffer space.
                             */
                            file_raw->hdr_item.size =
                                (uint32_t)(hdr_size + items_len);
                            *buf_len = file_raw->hdr_item.size;
                            ret = SWICC_RET_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    else
    {
        printf("File ADF: 'name' is missing or not of type: object.\n");
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse it as a DF and write the
 * parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_file_df;
static swicc_ret_et jsitem_prs_file_df(cJSON const *const item_json,
                                       uint32_t const offset_prel,
                                       uint8_t *const buf,
                                       uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    swicc_fs_file_raw_st *const file_raw = (swicc_fs_file_raw_st *)buf;
    swicc_fs_df_hdr_raw_st *const df_hdr_raw =
        (swicc_fs_df_hdr_raw_st *)file_raw->data;
    uint32_t const hdr_size =
        swicc_fs_item_hdr_raw_size[SWICC_FS_ITEM_TYPE_FILE_DF];

    cJSON *const name_obj = cJSON_GetObjectItemCaseSensitive(item_json, "name");
    if (name_obj != NULL && cJSON_IsObject(name_obj) == true)
    {
        /* Check that DF headers will fit inside the buffer. */
        if (*buf_len >= hdr_size)
        {
            /* Excess bytes in the AID should be 0. */
            memset(&df_hdr_raw->name, 0U, sizeof(df_hdr_raw->name));

            uint32_t name_len = sizeof(df_hdr_raw->name);
            ret = jsitem_prs_demux(name_obj, 0U /* Not used. */,
                                   df_hdr_raw->name, &name_len);
            if (ret == SWICC_RET_SUCCESS)
            {
                ret = jsitem_prs_file_raw(item_json, file_raw, offset_prel);
                if (ret == SWICC_RET_SUCCESS)
                {
                    /**
                     * Safe because buffer length is greater than header length.
                     */
                    uint32_t items_len = (uint32_t)(*buf_len - hdr_size);
                    ret = jsitem_prs_file_folder(item_json, hdr_size,
                                                 &buf[hdr_size], &items_len);
                    if (ret == SWICC_RET_SUCCESS)
                    {
                        file_raw->hdr_item.lcs = SWICC_FS_LCS_OPER_ACTIV;
                        file_raw->hdr_item.type = SWICC_FS_ITEM_TYPE_FILE_DF;
                        /**
                         * Safe cast because parsing would fail if there was not
                         * enough buffer space.
                         */
                        file_raw->hdr_item.size =
                            (uint32_t)(hdr_size + items_len);
                        *buf_len = file_raw->hdr_item.size;
                        ret = SWICC_RET_SUCCESS;
                    }
                }
            }
        }
        else
        {
            ret = SWICC_RET_BUFFER_TOO_SHORT;
        }
    }
    else
    {
        printf("File DF/MF: 'name' is missing or not of type: object.\n");
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse it as a transparent EF
 * and write the parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_file_ef_transparent;
static swicc_ret_et jsitem_prs_file_ef_transparent(cJSON const *const item_json,
                                                   uint32_t const offset_prel,
                                                   uint8_t *const buf,
                                                   uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    swicc_fs_file_raw_st *const file_raw = (swicc_fs_file_raw_st *)buf;
    __attribute__((unused))
    swicc_fs_ef_transparent_hdr_raw_st *const ef_hdr_raw =
        (swicc_fs_ef_transparent_hdr_raw_st *)file_raw->data;
    uint32_t const hdr_len =
        swicc_fs_item_hdr_raw_size[SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT];
    if (*buf_len >= hdr_len)
    {
        ret = jsitem_prs_file_raw(item_json, file_raw, offset_prel);
        if (ret == SWICC_RET_SUCCESS)
        {
            memcpy(buf, file_raw, hdr_len);
            /* Safe cast due to check of buffer length to header size. */
            uint32_t contents_len = (uint32_t)(*buf_len - hdr_len);
            swicc_ret_et ret_data = SWICC_RET_ERROR;
            cJSON *const contents_obj =
                cJSON_GetObjectItemCaseSensitive(item_json, "contents");
            if (contents_obj != NULL && cJSON_IsObject(contents_obj) == true)
            {
                /**
                 * In theory, this call to demux allows the contents of a
                 * transparent file to be of any item type but these items
                 * will become a byte array and will be interpreted as one
                 * by the FS anyways.
                 */
                ret_data = jsitem_prs_demux(contents_obj, hdr_len,
                                            &buf[hdr_len], &contents_len);
            }
            else if (contents_obj != NULL && cJSON_IsNull(contents_obj) == true)
            {
                contents_len = 0U;
                ret_data = SWICC_RET_SUCCESS;
            }
            else
            {
                printf("File EF transparent: 'contents' missing or not of "
                       "type: null, object.\n");
            }

            if (ret_data == SWICC_RET_SUCCESS)
            {
                file_raw->hdr_item.type =
                    SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT;
                file_raw->hdr_item.lcs = SWICC_FS_LCS_OPER_ACTIV;
                /* Safe cast because parsing would fail otherwise. */
                file_raw->hdr_item.size = (uint32_t)(hdr_len + contents_len);
                *buf_len = file_raw->hdr_item.size;
            }
            ret = ret_data;
        }
    }
    else
    {
        ret = SWICC_RET_BUFFER_TOO_SHORT;
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse it as a linear-fixed EF
 * and write the parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_file_ef_linearfixed;
static swicc_ret_et jsitem_prs_file_ef_linearfixed(cJSON const *const item_json,
                                                   uint32_t const offset_prel,
                                                   uint8_t *const buf,
                                                   uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    swicc_fs_file_raw_st *const file_raw = (swicc_fs_file_raw_st *)buf;
    swicc_fs_ef_linearfixed_hdr_raw_st *const ef_hdr_raw =
        (swicc_fs_ef_linearfixed_hdr_raw_st *)file_raw->data;
    uint32_t const hdr_len =
        swicc_fs_item_hdr_raw_size[SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED];
    if (*buf_len >= hdr_len)
    {
        ret = jsitem_prs_file_raw(item_json, file_raw, offset_prel);
        if (ret == SWICC_RET_SUCCESS)
        {
            cJSON *const rcrd_size_obj =
                cJSON_GetObjectItemCaseSensitive(item_json, "rcrd_size");
            if (rcrd_size_obj != NULL && cJSON_IsNumber(rcrd_size_obj) == true)
            {
                /**
                 * Forcing this cast because the number in the JSON should
                 * have been a natural number.
                 */
                uint8_t const rcrd_size =
                    (uint8_t)cJSON_GetNumberValue(rcrd_size_obj);
                ef_hdr_raw->rcrd_size = rcrd_size;

                cJSON *const contents_arr =
                    cJSON_GetObjectItemCaseSensitive(item_json, "contents");
                swicc_ret_et ret_item = SWICC_RET_ERROR;
                uint32_t contents_len = 0U; /* Parsed length. */
                if (contents_arr != NULL && cJSON_IsNull(contents_arr) == true)
                {
                    contents_len = 0U;
                    ret_item = SWICC_RET_SUCCESS;
                }
                else if (contents_arr != NULL &&
                         cJSON_IsArray(contents_arr) == true)
                {
                    cJSON *item = NULL;
                    cJSON_ArrayForEach(item, contents_arr)
                    {
                        /* Make sure another record can fit. */
                        if (hdr_len + contents_len + ef_hdr_raw->rcrd_size >
                            *buf_len)
                        {
                            ret_item = SWICC_RET_BUFFER_TOO_SHORT;
                            break;
                        }
                        /* Safe cast due to size check. */
                        uint32_t item_size =
                            (uint32_t)(*buf_len - (hdr_len + contents_len));
                        /**
                         * By default, unused space must be filled with 0xFF.
                         */
                        memset(&buf[hdr_len + contents_len], 0xFF,
                               ef_hdr_raw->rcrd_size);
                        ret_item = jsitem_prs_demux(
                            item, hdr_len + contents_len,
                            &buf[hdr_len + contents_len], &item_size);
                        if (ret_item != SWICC_RET_SUCCESS ||
                            item_size > ef_hdr_raw->rcrd_size)
                        {
                            ret_item = SWICC_RET_ERROR;
                            break;
                        }

                        /* Every item must have the same length. */
                        contents_len += ef_hdr_raw->rcrd_size;
                    }
                    /* This would mean the contents array is empty. */
                    if (item == NULL)
                    {
                        ret_item = SWICC_RET_SUCCESS;
                    }

                    /**
                     * Enusre the buffer is not overflown with the parsed items.
                     */
                    if (hdr_len + contents_len > *buf_len)
                    {
                        ret_item = SWICC_RET_BUFFER_TOO_SHORT;
                    }
                }
                else
                {
                    printf("File EF linear-fixed/cyclic: 'contents' missing or "
                           "not of type: null, array.\n");
                }

                /**
                 * The buffer size was already checked and the items + header
                 * will fit inside it.
                 */
                if (ret_item == SWICC_RET_SUCCESS)
                {
                    file_raw->hdr_item.type =
                        SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED;
                    file_raw->hdr_item.lcs = SWICC_FS_LCS_OPER_ACTIV;
                    /* Safe cast due to check on buffer length. */
                    file_raw->hdr_item.size =
                        (uint32_t)(hdr_len + contents_len);
                    *buf_len = file_raw->hdr_item.size;
                }
                ret = ret_item;
            }
            else
            {
                printf("File EF linear-fixed/cyclic: 'rcrd_size' missing or "
                       "not of type: number.\n");
            }
        }
    }
    else
    {
        ret = SWICC_RET_BUFFER_TOO_SHORT;
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse it as a cyclic EF and
 * write the parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_file_ef_cyclic;
static swicc_ret_et jsitem_prs_file_ef_cyclic(cJSON const *const item_json,
                                              uint32_t const offset_prel,
                                              uint8_t *const buf,
                                              uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et const ret = jsitem_prs[SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED](
        item_json, offset_prel, buf, buf_len);
    if (ret == SWICC_RET_SUCCESS)
    {
        swicc_fs_file_raw_st *file_raw = (swicc_fs_file_raw_st *)buf;
        file_raw->hdr_item.type = SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC;
    }
    return ret;
}

/**
 * @brief A helper for parsing BER-TLV DO items. This will parse (recursively),
 * all the DOs contained in the JSON-encoded BER-TLV DO and write the parsed
 * representation into the buffer.
 * @param bertlv_json This should be the object contained in the 'contents' of
 * a DO BER-TLV item or object in the 'val' field of a BER-TLV DO.
 * @param enc
 * @return Return code.
 */
static swicc_ret_et prs_bertlv(cJSON const *const bertlv_json,
                               swicc_dato_bertlv_enc_st *const enc)
{
    if (bertlv_json != NULL && cJSON_IsObject(bertlv_json) == true)
    {
        cJSON *const tag_obj =
            cJSON_GetObjectItemCaseSensitive(bertlv_json, "tag");
        cJSON *const val_obj =
            cJSON_GetObjectItemCaseSensitive(bertlv_json, "val");
        if (tag_obj != NULL && cJSON_IsObject(tag_obj) && val_obj != NULL &&
            (cJSON_IsString(val_obj) == true || cJSON_IsNull(val_obj) == true ||
             cJSON_IsArray(val_obj) == true))
        {
            cJSON *const tag_class_obj =
                cJSON_GetObjectItemCaseSensitive(tag_obj, "class");
            cJSON *const tag_number_obj =
                cJSON_GetObjectItemCaseSensitive(tag_obj, "number");
            if (tag_class_obj != NULL &&
                cJSON_IsNumber(tag_class_obj) == true &&
                tag_number_obj != NULL &&
                cJSON_IsNumber(tag_number_obj) == true)
            {
                bool const dato_constr = cJSON_IsArray(val_obj) == true;

                /* Make sure the class number is in a valid range. */
                double const cla_raw = cJSON_GetNumberValue(tag_class_obj);
                if (cla_raw > UINT32_MAX || cla_raw < 0)
                {
                    printf("Item dato BER-TLV: CLA is outside of the valid "
                           "range (got %lf, expected <%lf and >%lf).\n",
                           cla_raw, (double)UINT32_MAX, cla_raw);
                    return SWICC_RET_ERROR;
                }

                /* Safe casr since it was checked to be. */
                swicc_dato_bertlv_tag_cla_et cla;
                switch ((uint32_t)cla_raw)
                {
                case 0:
                    cla = SWICC_DATO_BERTLV_TAG_CLA_UNIVERSAL;
                    break;
                case 1:
                    cla = SWICC_DATO_BERTLV_TAG_CLA_APPLICATION;
                    break;
                case 2:
                    cla = SWICC_DATO_BERTLV_TAG_CLA_CONTEXT_SPECIFIC;
                    break;
                case 3:
                    cla = SWICC_DATO_BERTLV_TAG_CLA_PRIVATE;
                    break;
                default:
                    cla = SWICC_DATO_BERTLV_TAG_CLA_INVALID;
                    break;
                }
                swicc_dato_bertlv_tag_st const tag = {
                    .cla = cla,
                    .num = (uint32_t)cJSON_GetNumberValue(tag_number_obj),
                    .pc = dato_constr,
                };
                if (dato_constr)
                {
                    /**
                     * For constructed DOs, parse nested items backwards (from
                     * the last to the first) then encode header and return.
                     */
                    swicc_dato_bertlv_enc_st enc_nstd;

                    if (swicc_dato_bertlv_enc_nstd_start(enc, &enc_nstd) ==
                        SWICC_RET_SUCCESS)
                    {
                        swicc_ret_et ret_env = SWICC_RET_ERROR;
                        int32_t const val_obj_cnt = cJSON_GetArraySize(val_obj);
                        if (val_obj_cnt < 0)
                        {
                            /* Length of the value should never be negative. */
                            return SWICC_RET_ERROR;
                        }

                        /* Iterate from the last to the first element. */
                        /* Safe cast since count was checked to be positive. */
                        for (int32_t val_obj_idx = val_obj_cnt - 1;
                             val_obj_idx >= 0; --val_obj_idx)
                        {
                            /**
                             * Safe cast since the idx will never surpass a
                             * value of int32.
                             */
                            cJSON *const val_obj_nstd = cJSON_GetArrayItem(
                                val_obj, (int32_t)val_obj_idx);
                            ret_env = prs_bertlv(val_obj_nstd, &enc_nstd);
                            if (ret_env != SWICC_RET_SUCCESS)
                            {
                                break;
                            }
                        }
                        if (ret_env == SWICC_RET_SUCCESS)
                        {
                            if (swicc_dato_bertlv_enc_nstd_end(
                                    enc, &enc_nstd) == SWICC_RET_SUCCESS &&
                                swicc_dato_bertlv_enc_hdr(enc, &tag) ==
                                    SWICC_RET_SUCCESS)
                            {
                                return SWICC_RET_SUCCESS;
                            }
                        }
                    }
                }
                else
                {
                    /**
                     * For primitive DOs, encode that data then header and
                     * return.
                     */
                    if (cJSON_IsString(val_obj) == true)
                    {
                        char *const val_str = cJSON_GetStringValue(val_obj);
                        uint64_t const val_str_len = strlen(val_str);
                        if (val_str_len > UINT32_MAX)
                        {
                            /* Value length should never be this large. */
                            return SWICC_RET_ERROR;
                        }
                        swicc_ret_et ret_enc = SWICC_RET_ERROR;
                        /* Safe cast since len was checked to fit in uint32. */
                        uint32_t bytearr_len = (uint32_t)val_str_len / 2U;
                        uint8_t *bytearr = malloc(bytearr_len);
                        if (bytearr != NULL)
                        {
                            /* Safe cast since val len was checked. */
                            if (swicc_hexstr_bytearr(
                                    val_str, (uint32_t)val_str_len, bytearr,
                                    &bytearr_len) == SWICC_RET_SUCCESS)
                            {
                                if (swicc_dato_bertlv_enc_data(enc, bytearr,
                                                               bytearr_len) ==
                                        SWICC_RET_SUCCESS &&
                                    swicc_dato_bertlv_enc_hdr(enc, &tag) ==
                                        SWICC_RET_SUCCESS)
                                {
                                    ret_enc = SWICC_RET_SUCCESS;
                                }
                            }
                            else
                            {
                                printf("Item dato BER-TLV: Failed to convert "
                                       "value hex string to a byte array.\n");
                            }
                            free(bytearr);
                        }
                        if (ret_enc == SWICC_RET_SUCCESS)
                        {
                            return ret_enc;
                        }
                    }
                    else
                    {
                        /**
                         * If value is NULL, it just means the value has length
                         * 0.
                         */
                        if (swicc_dato_bertlv_enc_hdr(enc, &tag) ==
                            SWICC_RET_SUCCESS)
                        {
                            return SWICC_RET_SUCCESS;
                        }
                    }
                }
            }
            else
            {
                printf("Item dato BER-TLV: 'class' missing or not of type: "
                       "number, or 'number' missing or not of type: number.\n");
            }
        }
        else
        {
            printf("Item dato BER-TLV: 'tag' missing or not of type: null, "
                   "object, or 'val' missing or not of type: null, array.\n");
        }
    }
    return SWICC_RET_ERROR;
}

/**
 * @brief Given an item encoded as a JSON object, parse the contents as a
 * BER-TLV DO and write the parsed representation into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_item_dato_bertlv;
static swicc_ret_et jsitem_prs_item_dato_bertlv(cJSON const *const item_json,
                                                uint32_t const offset_prel,
                                                uint8_t *const buf,
                                                uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    cJSON *const contents_obj =
        cJSON_GetObjectItemCaseSensitive(item_json, "contents");
    if (contents_obj != NULL && cJSON_IsNull(contents_obj) == true)
    {
        *buf_len = 0U;
        ret = SWICC_RET_SUCCESS;
    }
    else if (contents_obj != NULL && cJSON_IsObject(contents_obj) == true)
    {
        swicc_dato_bertlv_enc_st enc;
        uint8_t *enc_buf;
        uint32_t enc_buf_len;
        swicc_ret_et ret_enc = SWICC_RET_ERROR;
        for (bool dry_run = true;; dry_run = false)
        {
            if (dry_run)
            {
                enc_buf = NULL;
                enc_buf_len = 0U;
            }
            else
            {
                if (enc.len > *buf_len)
                {
                    return SWICC_RET_BUFFER_TOO_SHORT;
                }
                enc_buf = buf;
                enc_buf_len = enc.len;
            }
            swicc_dato_bertlv_enc_init(&enc, enc_buf, enc_buf_len);
            ret_enc = prs_bertlv(contents_obj, &enc);
            if (ret_enc != SWICC_RET_SUCCESS)
            {
                break;
            }

            if (!dry_run)
            {
                ret_enc = SWICC_RET_SUCCESS;
                break;
            }
        }
        if (ret_enc == SWICC_RET_SUCCESS)
        {
            *buf_len = enc_buf_len;
        }
        else
        {
            printf("Item dato BER-TLV: Failed to parse the JSON representation "
                   "into a BER-TLV DO.\n");
        }
        ret = ret_enc;
    }
    else
    {
        printf("Item dato BER-TLV: 'contents' missing or not of type: null, "
               "object.\n");
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, parse the contents into the
 * given buffer by converting the hex string into a byte array.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_item_hex;
static swicc_ret_et jsitem_prs_item_hex(cJSON const *const item_json,
                                        uint32_t const offset_prel,
                                        uint8_t *const buf,
                                        uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    cJSON *const contents_obj =
        cJSON_GetObjectItemCaseSensitive(item_json, "contents");
    if (contents_obj != NULL && cJSON_IsNull(contents_obj) == true)
    {
        *buf_len = 0U;
        ret = SWICC_RET_SUCCESS;
    }
    else if (contents_obj != NULL && cJSON_IsString(contents_obj) == true)
    {
        char *const contents_str = cJSON_GetStringValue(contents_obj);
        if (contents_str != NULL)
        {
            uint64_t const hexstr_len = strlen(contents_str);
            if (hexstr_len <= UINT32_MAX && hexstr_len % 2U == 0U)
            {
                uint32_t bytearr_len = *buf_len;
                /* Safe cast due to the boundary check. */
                ret = swicc_hexstr_bytearr(contents_str,
                                           (uint32_t)strlen(contents_str), buf,
                                           &bytearr_len);
                if (ret == SWICC_RET_SUCCESS)
                {
                    if (*buf_len >= bytearr_len)
                    {
                        *buf_len = bytearr_len;
                    }
                    else
                    {
                        ret = SWICC_RET_BUFFER_TOO_SHORT;
                    }
                }
                else
                {
                    printf("Item hex: Failed to convert hex string to a byte "
                           "array.\n");
                }
            }
            else
            {
                printf("Item hex: hex string has an invalid length (got %lu, "
                       "expected <=%u and multiple of 2).\n",
                       hexstr_len, UINT32_MAX);
            }
        }
    }
    else
    {
        printf("Item hex: 'contents' missing or not of type: null, string.\n");
    }
    return ret;
}

/**
 * @brief Given an item encoded as a JSON object, write the ASCII text contained
 * in the contents, into the buffer.
 * @param item_json
 * @param offset_prel
 * @param buf
 * @param buf_len Shall contain the length of the buffer. It will receive the
 * size of the parsed representation.
 * @return Return code.
 */
static jsitem_prs_ft jsitem_prs_item_ascii;
static swicc_ret_et jsitem_prs_item_ascii(cJSON const *const item_json,
                                          uint32_t const offset_prel,
                                          uint8_t *const buf,
                                          uint32_t *const buf_len)
{
    if (item_json == NULL || cJSON_IsObject(item_json) != true || buf == NULL ||
        buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_ERROR;
    cJSON *const contents_obj =
        cJSON_GetObjectItemCaseSensitive(item_json, "contents");
    if (contents_obj != NULL && cJSON_IsNull(contents_obj) == true)
    {
        *buf_len = 0U;
        ret = SWICC_RET_SUCCESS;
    }
    else if (contents_obj != NULL && cJSON_IsString(contents_obj) == true)
    {
        char *const contents_str = cJSON_GetStringValue(contents_obj);
        if (contents_str != NULL)
        {
            uint64_t const ascii_len = strlen(contents_str);
            if (ascii_len <= UINT32_MAX)
            {
                if (ascii_len <= *buf_len)
                {
                    memcpy(buf, contents_str, ascii_len);
                    *buf_len = (uint32_t)
                        ascii_len; /* Safe cast due to the boundary check. */
                    ret = SWICC_RET_SUCCESS;
                }
                else
                {
                    ret = SWICC_RET_BUFFER_TOO_SHORT;
                }
            }
            else
            {
                printf("Item ASCII: ASCII string is too long (got %lu, "
                       "expected <=%u).\n",
                       ascii_len, UINT32_MAX);
            }
        }
    }
    else
    {
        printf(
            "Item ASCII: 'contents' missing or not of type: null, string.\n");
    }
    return ret;
}

/**
 * Provide a convenient lookup table for parsers for each item type.
 */
static jsitem_prs_ft *const jsitem_prs[] = {
    [SWICC_FS_ITEM_TYPE_FILE_MF] = jsitem_prs_file_mf,
    [SWICC_FS_ITEM_TYPE_FILE_ADF] = jsitem_prs_file_adf,
    [SWICC_FS_ITEM_TYPE_FILE_DF] = jsitem_prs_file_df,
    [SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT] = jsitem_prs_file_ef_transparent,
    [SWICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED] = jsitem_prs_file_ef_linearfixed,
    [SWICC_FS_ITEM_TYPE_FILE_EF_CYCLIC] = jsitem_prs_file_ef_cyclic,
    [SWICC_FS_ITEM_TYPE_DATO_BERTLV] = jsitem_prs_item_dato_bertlv,
    [SWICC_FS_ITEM_TYPE_HEX] = jsitem_prs_item_hex,
    [SWICC_FS_ITEM_TYPE_ASCII] = jsitem_prs_item_ascii,
};

/**
 * @brief Parse the JSON of a disk into an in-memory representation (swICC FS
 * disk format).
 * @param disk Disk structure to populate.
 * @param disk_json Disk in JSON format.
 * @return Return code.
 */
static swicc_ret_et disk_json_prs(swicc_disk_st *const disk,
                                  cJSON const *const disk_json)
{
    if (disk == NULL || disk_json == NULL || cJSON_IsObject(disk_json) != true)
    {
        return SWICC_RET_PARAM_BAD;
    }

    swicc_ret_et ret = SWICC_RET_SUCCESS;
    if (disk->root != NULL)
    {
        printf("Root: old disk must be unloaded first.\n");
        return SWICC_RET_ERROR;
    }

    uint8_t tree_count = 0U;
    cJSON *const disk_obj = cJSON_GetObjectItemCaseSensitive(disk_json, "disk");
    if (disk_obj != NULL && cJSON_IsArray(disk_obj) == true)
    {
        cJSON *tree_obj;
        swicc_disk_tree_st *tree = NULL;
        cJSON_ArrayForEach(tree_obj, disk_obj)
        {
            /**
             * Check if this is the first item in the root. If so, add a
             * reference to the forest of trees (linked list) to the disk root.
             */
            if (tree == NULL)
            {
                tree = malloc(sizeof(*tree));
                if (tree == NULL)
                {
                    printf("Tree: failed to allocate space for a tree struct "
                           "for the root tree.\n");
                    /* Nothing should have been allocated before. */
                    ret = SWICC_RET_ERROR;
                    break;
                }
                disk->root = tree;
                memset(disk->root, 0U, sizeof(*disk->root));
            }
            else
            {
                tree->next = malloc(sizeof(*tree));
                if (tree->next == NULL)
                {
                    printf("Tree: failed to allocate a tree struct for next "
                           "tree.\n");
                    ret = SWICC_RET_ERROR;
                    break;
                }
                tree = tree->next;
                memset(tree, 0U, sizeof(*tree));
            }

            memset(tree, 0U, sizeof(*tree));
            tree->buf = malloc(DISK_SIZE_START);
            if (tree->buf == NULL)
            {
                printf("Tree: failed to allocate a tree buffer.\n");
                ret = SWICC_RET_ERROR;
                break;
            }
            tree->size = DISK_SIZE_START;
            tree->len = 0U;

            uint32_t item_size;
            do
            {
                item_size = tree->size - tree->len;
                ret = jsitem_prs_demux(tree_obj, 0U, &tree->buf[tree->len],
                                       &item_size);
                if (ret == SWICC_RET_BUFFER_TOO_SHORT)
                {
                    uint8_t *const buf_new =
                        realloc(tree->buf, tree->size + DISK_SIZE_RESIZE);
                    if (buf_new != NULL)
                    {
                        uint64_t const tree_buf_size_new =
                            tree->size + DISK_SIZE_RESIZE;
                        if (tree_buf_size_new > UINT32_MAX)
                        {
                            printf(
                                "Tree: buffer size limit has been reached.\n");
                            ret = SWICC_RET_ERROR;
                            /**
                             * No break because we still have to update the tree
                             * so it gets properly freed later.
                             */
                        }
                        tree->buf = buf_new;
                        /**
                         * Safe cast due to the bound check against uint32 max.
                         */
                        tree->size = (uint32_t)tree_buf_size_new;
                    }
                    else
                    {
                        printf("Tree: failed to realloc tree buffer from %u "
                               "bytes to %u bytes.\n",
                               tree->size, tree->size + DISK_SIZE_RESIZE);
                        ret = SWICC_RET_ERROR;
                        break;
                    }
                }
                else if (ret != SWICC_RET_SUCCESS)
                {
                    printf("Tree: failed to parse tree JSON: %s.\n",
                           swicc_dbg_ret_str(ret));
                }
            } while (ret == SWICC_RET_BUFFER_TOO_SHORT);
            if (ret != SWICC_RET_SUCCESS)
            {
                printf("Tree: failed to parse tree contents: %s.\n",
                       swicc_dbg_ret_str(ret));
                break;
            }

            /**
             * Each tree contains exactly one file so the tree length equals to
             * the root element length. Safe cast due to check to see if it
             * overflows the type.
             */
            tree->len = item_size;

            ret = swicc_disk_lutsid_rebuild(disk, tree);
            if (ret != SWICC_RET_SUCCESS)
            {
                /**
                 * No need to clean up SID LUT since this will be done when
                 * whole root get emptied due to this error.
                 */
                printf("Tree: failed to create the SID LUT: %s.\n",
                       swicc_dbg_ret_str(ret));
                break;
            }

            /**
             * Unsafe case which relies on there being fewer than 256 trees in
             * the root.
             */
            tree_count = (uint8_t)(tree_count + 1U);
        }

        /* Make sure the forest has been created successfully. */
        if (ret != SWICC_RET_SUCCESS)
        {
            swicc_disk_root_empty(disk);
            memset(disk, 0U, sizeof(*disk));
            ret = SWICC_RET_ERROR;
            printf("Root: failed to create the forest of trees (parsed %u "
                   "trees): %s.\n",
                   tree_count, swicc_dbg_ret_str(ret));
        }
        ret = swicc_disk_lutid_rebuild(disk);
        if (ret != SWICC_RET_SUCCESS)
        {
            swicc_disk_lutid_empty(disk);
        }
    }
    else
    {
        printf("Root: 'disk' missing or not of type: object.\n");
        ret = SWICC_RET_ERROR;
    }
    return ret;
}

swicc_ret_et swicc_diskjs_disk_create(swicc_disk_st *const disk,
                                      char const *const disk_json_path)
{
    /* Need to unload old disk to create a new one in its place. */
    if (disk->root != NULL || disk->lutid.buf1 != NULL ||
        disk->lutid.buf2 != NULL)
    {
        return SWICC_RET_ERROR;
    }
    memset(disk, 0U, sizeof(*disk));
    swicc_ret_et ret = SWICC_RET_ERROR;
    FILE *f = fopen(disk_json_path, "rb");
    if (f != NULL)
    {
        /* Seek to end. */
        if (fseek(f, 0, SEEK_END) == 0)
        {
            /* Get current position of read pointer to get file size. */
            int64_t f_len_tmp = ftell(f);
            if (f_len_tmp >= 0 && f_len_tmp <= UINT32_MAX)
            {
                if (fseek(f, 0, SEEK_SET) == 0)
                {
                    /* Safe cast due to the size checks before. */
                    uint32_t const disk_json_raw_len = (uint32_t)f_len_tmp;
                    char *const disk_json_raw = malloc(disk_json_raw_len);
                    if (disk_json_raw != NULL)
                    {
                        if (fread(disk_json_raw, 1U, disk_json_raw_len, f) ==
                            disk_json_raw_len)
                        {
                            cJSON *const disk_json = cJSON_ParseWithLength(
                                disk_json_raw, disk_json_raw_len);
                            if (disk_json != NULL)
                            {
                                ret = disk_json_prs(disk, disk_json);
                                cJSON_Delete(disk_json);
                            }
                        }
                    }
                    free(disk_json_raw);
                }
            }
        }
        if (fclose(f) != 0)
        {
            ret = SWICC_RET_ERROR;
        }
    }
    return ret;
}

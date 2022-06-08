#include <string.h>
#include <uicc/uicc.h>

/**
 * @brief Create a file descriptor byte for a file.
 * @param file
 * @param file_descr
 * @return Return code.
 * @note Done according to ISO 7816-4:2020 p.29 sec.7.4.5 table.12.
 */
static uicc_ret_et uicc_fs_file_descr_byte(uicc_fs_file_st const *const file,
                                           uint8_t *const file_descr)
{
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }
    /**
     * 0
     *  0       = File not shareable
     *   xxx    = DF or EF category
     *      xxx = 0 for DF or EF structure
     */
    *file_descr = 0b00000000;
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_MF ||
        file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_ADF ||
        file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_DF)
    {
        *file_descr |= 0b00111000;
    }
    else
    {
        /**
         * TODO: Not sure what working and internal EFs are so just setting it
         * to "internal EF" i.e. EF for storing data interpreted by the card.
         */
        *file_descr |= 0b00001000;
        switch (file->hdr_item.type)
        {
        case UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT:
            *file_descr |= 0b00000001;
            break;
        case UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED:
            *file_descr |= 0b00000010;
            break;
        case UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC:
            *file_descr |= 0b00000110;
            break;
        default:
            return UICC_RET_PARAM_BAD;
        }
    }
    return UICC_RET_SUCCESS;
}

/**
 * @brief Create a data coding byte for a file.
 * @param file
 * @param data_coding
 * @return Return code.
 * @note Done according to second software function table described in ISO
 * 7816-4:2020 p.123 sec.12.2.2.9 table.126.
 */
static uicc_ret_et uicc_fs_file_data_coding(uicc_fs_file_st const *const file,
                                            uint8_t *const data_coding)
{
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }
    /**
     * 0        = EFs of BER-TLV structure supported.
     *  01      = Behavior of write function is proprietary.
     *    0     = Value 'FF' for first byte of BER-TLV tag fields is invalid.
     *     0001 = Data unit size is 2 quartet = 1 byte.
     */
    *data_coding = 0b00100001;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_fs_disk_mount(uicc_st *const uicc_state,
                               uicc_disk_st *const disk)
{
    if (uicc_state->fs.disk.root == NULL)
    {
        memcpy(&uicc_state->fs.disk, disk, sizeof(*disk));
        return UICC_RET_SUCCESS;
    }
    return UICC_RET_ERROR;
}

uicc_ret_et uicc_fs_file_lcs(uicc_fs_file_st const *const file,
                             uint8_t *const lcs)
{
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_INVALID)
    {
        return UICC_RET_PARAM_BAD;
    }
    switch (file->hdr_item.lcs)
    {
    // case UICC_FS_LCS_NINFO:
    //     *lcs = 0b00000000;
    //     break;
    // case UICC_FS_LCS_CREAT:
    //     *lcs = 0b00000001;
    //     break;
    // case UICC_FS_LCS_INIT:
    //     *lcs = 0b00000011;
    //     break;
    case UICC_FS_LCS_OPER_ACTIV:
        *lcs = 0b00000101;
        break;
    case UICC_FS_LCS_OPER_DEACTIV:
        *lcs = 0b00000100;
        break;
    case UICC_FS_LCS_TERM:
        *lcs = 0b00001100;
        break;
    }
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_fs_file_descr(
    uicc_disk_tree_st const *const tree, uicc_fs_file_st const *const file,
    uint8_t buf[static const UICC_FS_FILE_DESCR_LEN_MAX],
    uint8_t *const descr_len)
{
    uicc_ret_et const ret_descr_byte = uicc_fs_file_descr_byte(file, &buf[0U]);
    uicc_ret_et const ret_coding = uicc_fs_file_data_coding(file, &buf[1U]);
    uint16_t rcrd_len = 0U;
    uint32_t rcrd_cnt = 0U;
    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED)
    {
        rcrd_len = file->hdr_spec.ef_linearfixed.rcrd_size;
        uicc_disk_file_rcrd_cnt(tree, file, &rcrd_cnt);
    }
    else if (file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        rcrd_len = file->hdr_spec.ef_cyclic.rcrd_size;
        uicc_disk_file_rcrd_cnt(tree, file, &rcrd_cnt);
    }

    if (file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_LINEARFIXED ||
        file->hdr_item.type == UICC_FS_ITEM_TYPE_FILE_EF_CYCLIC)
    {
        *(uint16_t *)&buf[2U] = rcrd_len;
        if (rcrd_cnt > UINT8_MAX)
        {
            return UICC_RET_ERROR;
        }
        buf[4U] = (uint8_t)rcrd_cnt;
        *descr_len = 5U;
    }
    else
    {
        *descr_len = 2U;
    }

    if (ret_descr_byte == UICC_RET_SUCCESS && ret_coding == UICC_RET_SUCCESS)
    {
        return UICC_RET_SUCCESS;
    }
    else
    {
        return UICC_RET_ERROR;
    }
}

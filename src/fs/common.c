#include "uicc.h"
#include <string.h>

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
    item_hdr->offset_prel = item_hdr_raw->offset_prel;
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

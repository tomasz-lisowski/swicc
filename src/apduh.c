#include "uicc.h"
#include "uicc/apdu.h"
#include <stddef.h>
#include <stdio.h>

/**
 * Store pointers to handlers for every instruction in the interindustry class.
 */
static uicc_apduh_ft *const uicc_apduh[0xFF + 1U];

/**
 * @brief Handle both invalid and unknown instructions.
 * @param uicc_state
 * @param cmd
 * @param res
 * @return Return code.
 */
static uicc_apduh_ft apduh_unk;
static uicc_ret_et apduh_unk(uicc_st *const uicc_state,
                             uicc_apdu_cmd_st const *const cmd,
                             uicc_apdu_res_st *const res)
{
    res->sw1 = UICC_APDU_SW1_CHER_INS;
    res->sw2 = 0;
    res->data.len = 0;
    return UICC_RET_SUCCESS;
}

/**
 * @brief Handle the SELECT command in the interindustry class.
 * @param uicc_state
 * @param cmd
 * @param res
 * @return Return code.
 * @note As described in ISO 7816-4:2020 p.74 sec.11.2.2.
 */
static uicc_apduh_ft apduh_select;
static uicc_ret_et apduh_select(uicc_st *const uicc_state,
                                uicc_apdu_cmd_st const *const cmd,
                                uicc_apdu_res_st *const res)
{
    /**
     * Check if we only got Lc which means we need to send back a procedure
     * byte.
     */
    if (cmd->data->len == 0U)
    {
        /**
         * If Lc is 0 it means data is absent so we can process what we got.
         */
        if (*cmd->p3 > 0)
        {
            res->sw1 = UICC_APDU_SW1_PROC_ACK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }
    }
    uint8_t const lc = *cmd->p3;
    if (lc != cmd->data->len)
    {
        res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
        res->sw2 = 0x80; /* "Incorrect parameters in the command data field" */
        res->data.len = 0U;
        return UICC_RET_SUCCESS;
    }

    enum meth_e
    {
        METH_RFU,

        METH_MF_DF_EF,  /* Select MF, DF, or EF. Data: file ID or absent.
                         */
        METH_DF_NESTED, /* Select child DF. Data: file ID referencing a DF. */
        METH_EF_NESTED, /* Select EF under the DF referenced by 'current DF'.
                           Data: file ID referencing an EF. */
        METH_DF_PARENT, /* Select parent DF of the DF referenced by 'current
                           DF'. Data: absent. */

        METH_DF_NAME, /* Select by DF name. Data: E.g. App ID. */
        METH_MF_PATH, /* Select from the MF. Data: Path without the MF ID. */
        METH_DF_PATH, /* Select from the DF referenced by 'current DF'. Data:
                         path without the file ID of the DF referenced by
                         'current DF'. */

        METH_DO, /* Select a DO in the template referenced by the 'current
                    constructed DO'. Data: Tag belonging to the template
                    referenced by 'current constructed DO'. */
        METH_DO_PARENT, /* Select parent DO of the constructed DO setting the
                           template referenced by 'current constructed DO'.
                           Data: Absent. */
    } meth = METH_RFU;

    enum occ_e
    {
        OCC_RFU,

        OCC_FIRST, /* First or only occurrence. */
        OCC_LAST,  /* Last occurrence. */
        OCC_NEXT,  /* Next occurrence. */
        OCC_PREV,  /* Previous occurrence. */
    } occ = OCC_RFU;

    enum data_req_e
    {
        DATA_REQ_RFU,

        DATA_REQ_FCI, /* Return FCI template. Optional use of FCI tag and
                         length. */
        DATA_REQ_CP,  /* Return CP template. Mandatory use of CP tag and length.
                       */
        DATA_REQ_FMD, /* Return FMD template. Mandatory use of FMD tag and
                         length. */
        DATA_REQ_TAGS,   /* Return the tags belonging to the template set
                           by the selection of a constructed DO as a tag list. */
        DATA_REQ_ABSENT, /* No response data if Le is absent or proprietary Le
                        field present. */
    } data_req = DATA_REQ_RFU;

    /* Parse the command parameters. */
    {
        /* Decode P1. */
        switch (cmd->hdr->p1)
        {
        case 0b00000000:
            meth = METH_MF_DF_EF;
            break;
        case 0b00000001:
            meth = METH_DF_NESTED;
            break;
        case 0b00000010:
            meth = METH_EF_NESTED;
            break;
        case 0b00000011:
            meth = METH_DF_PARENT;
            break;
        case 0b00000100:
            meth = METH_DF_NAME;
            break;
        case 0b00001000:
            meth = METH_MF_PATH;
            break;
        case 0b00001001:
            meth = METH_DF_PATH;
            break;
        case 0b00010000:
            meth = METH_DO;
            break;
        case 0b00010011:
            meth = METH_DO_PARENT;
            break;
        default:
            meth = METH_RFU;
            break;
        }

        /* Decode P2. */
        switch (cmd->hdr->p2 & 0b11110011)
        {
        case 0b00000000:
            occ = OCC_FIRST;
            break;
        case 0b00000001:
            occ = OCC_LAST;
            break;
        case 0b00000010:
            occ = OCC_NEXT;
            break;
        case 0b00000011:
            occ = OCC_PREV;
            break;
        default:
            occ = OCC_RFU;
            break;
        }
        switch (cmd->hdr->p2 & 0b11111100)
        {
        case 0b00000000:
            data_req = DATA_REQ_FCI;
            break;
        case 0b00000100:
            data_req = DATA_REQ_CP;
            break;
        case 0b00001000:
            if (meth == METH_DO || meth == METH_DO_PARENT)
            {
                data_req = DATA_REQ_TAGS;
            }
            else
            {
                data_req = DATA_REQ_FMD;
            }
            break;
        case 0b00001100:
            data_req = DATA_REQ_ABSENT;
            break;
        default:
            data_req = DATA_REQ_RFU;
            break;
        }
    }

    /* Perform the requested action. */
    {
        /* Unsupported P1/P2 parameters. */
        if (meth == METH_RFU || data_req == DATA_REQ_RFU || occ == OCC_RFU ||
            meth == METH_DO || meth == METH_DO_PARENT)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        uicc_ret_et ret_select = UICC_RET_ERROR;
        switch (meth)
        {
        case METH_MF_DF_EF:
            /* Must contain exactly 1 file ID. */
            if (cmd->data->len != sizeof(uicc_fs_id_kt))
            {
                ret_select = UICC_RET_ERROR;
            }
            else
            {
                ret_select = uicc_fs_select_file_id(
                    uicc_state, *(uicc_fs_id_kt *)cmd->data->b);
            }
            break;
        case METH_DF_NESTED:
        case METH_EF_NESTED:
        case METH_DF_PARENT:
            ret_select = UICC_RET_ERROR;
            break;
        case METH_DF_NAME:
            /* Name must be at least 1 char long. */
            if (cmd->data->len == 0 || occ != OCC_FIRST)
            {
                ret_select = UICC_RET_ERROR;
            }
            else
            {
                ret_select = uicc_fs_select_file_dfname(
                    uicc_state, (char *)cmd->data->b, cmd->data->len);
            }
            break;
        case METH_MF_PATH:
            /* Must contain at least 1 ID in the path. */
            if (cmd->data->len < sizeof(uicc_fs_id_kt) || occ != OCC_FIRST)
            {
                ret_select = UICC_RET_ERROR;
            }
            else
            {
                uicc_fs_path_st const path = {
                    .b = cmd->data->b,
                    .len = cmd->data->len,
                    .type = UICC_FS_PATH_TYPE_MF,
                };
                ret_select = uicc_fs_select_file_path(uicc_state, path);
            }
            break;
        case METH_DF_PATH:
            /* Must contain at least 1 ID in the path. */
            if (cmd->data->len < sizeof(uicc_fs_id_kt) || occ != OCC_FIRST)
            {
                ret_select = UICC_RET_ERROR;
            }
            else
            {
                uicc_fs_path_st const path = {
                    .b = cmd->data->b,
                    .len = cmd->data->len,
                    .type = UICC_FS_PATH_TYPE_DF,
                };
                ret_select = uicc_fs_select_file_path(uicc_state, path);
            }
            break;
        default:
            /* Unreachable due to the parameter rejection done before. */
            __builtin_unreachable();
        }

        /* Respond with failure by default. */
        res->sw1 = UICC_APDU_SW1_CHER_UNK;
        res->sw2 = 0U;
        res->data.len = 0U;

        if (ret_select == UICC_RET_FS_NOT_FOUND)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x82; /* Not found. */
            res->data.len = 0U;
        }
        else if (ret_select == UICC_RET_SUCCESS)
        {
            uicc_fs_file_hdr_st *file_selected = NULL;
            if (uicc_state->internal.fs.va.cur_ef.item.type !=
                UICC_FS_ITEM_TYPE_INVALID)
            {
                file_selected = &uicc_state->internal.fs.va.cur_ef;
            }
            else if (uicc_state->internal.fs.va.cur_df.item.type !=
                     UICC_FS_ITEM_TYPE_INVALID)
            {
                file_selected = &uicc_state->internal.fs.va.cur_df;
            }

            if (file_selected != NULL)
            {
                res->sw1 = UICC_APDU_SW1_NORM_NONE;
                res->sw2 = 0U;
                res->data.len = 0U;
                switch (data_req)
                {
                case DATA_REQ_FCI:
                    break;
                case DATA_REQ_CP:
                    break;
                case DATA_REQ_FMD:
                    break;
                case DATA_REQ_ABSENT:
                    break;
                default:
                    /**
                     * Unreachable due to the parameter rejection done before.
                     */
                    __builtin_unreachable();
                }
            }
        }
        return UICC_RET_SUCCESS;
    }
}

uicc_ret_et uicc_apduh_pro_register(uicc_st *const uicc_state,
                                    uicc_apduh_ft *const handler)
{
    uicc_state->internal.apduh_pro = handler;
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_apduh_demux(uicc_st *const uicc_state,
                             uicc_apdu_cmd_st const *const cmd,
                             uicc_apdu_res_st *const res)
{
    uicc_ret_et ret = UICC_RET_APDU_UNHANDLED;
    switch (cmd->hdr->cla.type)
    {
    case UICC_APDU_CLA_TYPE_INVALID:
    case UICC_APDU_CLA_TYPE_RFU:
        res->sw1 = UICC_APDU_SW1_CHER_CLA; /* Marked as unsupported class. */
        res->sw2 = 0;
        res->data.len = 0;
        ret = UICC_RET_SUCCESS;
        break;
    case UICC_APDU_CLA_TYPE_INTERINDUSTRY:
        ret = uicc_apduh[cmd->hdr->ins](uicc_state, cmd, res);
        break;
    case UICC_APDU_CLA_TYPE_PROPRIETARY:
        if (uicc_state->internal.apduh_pro == NULL)
        {
            ret = UICC_RET_APDU_UNHANDLED;
            break;
        }
        return uicc_state->internal.apduh_pro(uicc_state, cmd, res);
    default:
        ret = UICC_RET_APDU_UNHANDLED;
        break;
    }

    if (ret == UICC_RET_APDU_UNHANDLED)
    {
        ret = UICC_RET_SUCCESS;
        res->sw1 = UICC_APDU_SW1_CHER_INS;
        res->sw2 = 0;
        res->data.len = 0;
    }
    return ret;
}

static uicc_apduh_ft *const uicc_apduh[0xFF + 1U] = {
    [0x00] = apduh_unk, [0x01] = apduh_unk, [0x02] = apduh_unk,
    [0x03] = apduh_unk, [0x04] = apduh_unk, [0x05] = apduh_unk,
    [0x06] = apduh_unk, [0x07] = apduh_unk, [0x08] = apduh_unk,
    [0x09] = apduh_unk, [0x0A] = apduh_unk, [0x0B] = apduh_unk,
    [0x0C] = apduh_unk, [0x0D] = apduh_unk, [0x0E] = apduh_unk,
    [0x0F] = apduh_unk, [0x10] = apduh_unk, [0x11] = apduh_unk,
    [0x12] = apduh_unk, [0x13] = apduh_unk, [0x14] = apduh_unk,
    [0x15] = apduh_unk, [0x16] = apduh_unk, [0x17] = apduh_unk,
    [0x18] = apduh_unk, [0x19] = apduh_unk, [0x1A] = apduh_unk,
    [0x1B] = apduh_unk, [0x1C] = apduh_unk, [0x1D] = apduh_unk,
    [0x1E] = apduh_unk, [0x1F] = apduh_unk, [0x20] = apduh_unk,
    [0x21] = apduh_unk, [0x22] = apduh_unk, [0x23] = apduh_unk,
    [0x24] = apduh_unk, [0x25] = apduh_unk, [0x26] = apduh_unk,
    [0x27] = apduh_unk, [0x28] = apduh_unk, [0x29] = apduh_unk,
    [0x2A] = apduh_unk, [0x2B] = apduh_unk, [0x2C] = apduh_unk,
    [0x2D] = apduh_unk, [0x2E] = apduh_unk, [0x2F] = apduh_unk,
    [0x30] = apduh_unk, [0x31] = apduh_unk, [0x32] = apduh_unk,
    [0x33] = apduh_unk, [0x34] = apduh_unk, [0x35] = apduh_unk,
    [0x36] = apduh_unk, [0x37] = apduh_unk, [0x38] = apduh_unk,
    [0x39] = apduh_unk, [0x3A] = apduh_unk, [0x3B] = apduh_unk,
    [0x3C] = apduh_unk, [0x3D] = apduh_unk, [0x3E] = apduh_unk,
    [0x3F] = apduh_unk, [0x40] = apduh_unk, [0x41] = apduh_unk,
    [0x42] = apduh_unk, [0x43] = apduh_unk, [0x44] = apduh_unk,
    [0x45] = apduh_unk, [0x46] = apduh_unk, [0x47] = apduh_unk,
    [0x48] = apduh_unk, [0x49] = apduh_unk, [0x4A] = apduh_unk,
    [0x4B] = apduh_unk, [0x4C] = apduh_unk, [0x4D] = apduh_unk,
    [0x4E] = apduh_unk, [0x4F] = apduh_unk, [0x50] = apduh_unk,
    [0x51] = apduh_unk, [0x52] = apduh_unk, [0x53] = apduh_unk,
    [0x54] = apduh_unk, [0x55] = apduh_unk, [0x56] = apduh_unk,
    [0x57] = apduh_unk, [0x58] = apduh_unk, [0x59] = apduh_unk,
    [0x5A] = apduh_unk, [0x5B] = apduh_unk, [0x5C] = apduh_unk,
    [0x5D] = apduh_unk, [0x5E] = apduh_unk, [0x5F] = apduh_unk,
    [0x60] = apduh_unk, [0x61] = apduh_unk, [0x62] = apduh_unk,
    [0x63] = apduh_unk, [0x64] = apduh_unk, [0x65] = apduh_unk,
    [0x66] = apduh_unk, [0x67] = apduh_unk, [0x68] = apduh_unk,
    [0x69] = apduh_unk, [0x6A] = apduh_unk, [0x6B] = apduh_unk,
    [0x6C] = apduh_unk, [0x6D] = apduh_unk, [0x6E] = apduh_unk,
    [0x6F] = apduh_unk, [0x70] = apduh_unk, [0x71] = apduh_unk,
    [0x72] = apduh_unk, [0x73] = apduh_unk, [0x74] = apduh_unk,
    [0x75] = apduh_unk, [0x76] = apduh_unk, [0x77] = apduh_unk,
    [0x78] = apduh_unk, [0x79] = apduh_unk, [0x7A] = apduh_unk,
    [0x7B] = apduh_unk, [0x7C] = apduh_unk, [0x7D] = apduh_unk,
    [0x7E] = apduh_unk, [0x7F] = apduh_unk, [0x80] = apduh_unk,
    [0x81] = apduh_unk, [0x82] = apduh_unk, [0x83] = apduh_unk,
    [0x84] = apduh_unk, [0x85] = apduh_unk, [0x86] = apduh_unk,
    [0x87] = apduh_unk, [0x88] = apduh_unk, [0x89] = apduh_unk,
    [0x8A] = apduh_unk, [0x8B] = apduh_unk, [0x8C] = apduh_unk,
    [0x8D] = apduh_unk, [0x8E] = apduh_unk, [0x8F] = apduh_unk,
    [0x90] = apduh_unk, [0x91] = apduh_unk, [0x92] = apduh_unk,
    [0x93] = apduh_unk, [0x94] = apduh_unk, [0x95] = apduh_unk,
    [0x96] = apduh_unk, [0x97] = apduh_unk, [0x98] = apduh_unk,
    [0x99] = apduh_unk, [0x9A] = apduh_unk, [0x9B] = apduh_unk,
    [0x9C] = apduh_unk, [0x9D] = apduh_unk, [0x9E] = apduh_unk,
    [0x9F] = apduh_unk, [0xA0] = apduh_unk, [0xA1] = apduh_unk,
    [0xA2] = apduh_unk, [0xA3] = apduh_unk, [0xA4] = apduh_select,
    [0xA5] = apduh_unk, [0xA6] = apduh_unk, [0xA7] = apduh_unk,
    [0xA8] = apduh_unk, [0xA9] = apduh_unk, [0xAA] = apduh_unk,
    [0xAB] = apduh_unk, [0xAC] = apduh_unk, [0xAD] = apduh_unk,
    [0xAE] = apduh_unk, [0xAF] = apduh_unk, [0xB0] = apduh_unk,
    [0xB1] = apduh_unk, [0xB2] = apduh_unk, [0xB3] = apduh_unk,
    [0xB4] = apduh_unk, [0xB5] = apduh_unk, [0xB6] = apduh_unk,
    [0xB7] = apduh_unk, [0xB8] = apduh_unk, [0xB9] = apduh_unk,
    [0xBA] = apduh_unk, [0xBB] = apduh_unk, [0xBC] = apduh_unk,
    [0xBD] = apduh_unk, [0xBE] = apduh_unk, [0xBF] = apduh_unk,
    [0xC0] = apduh_unk, [0xC1] = apduh_unk, [0xC2] = apduh_unk,
    [0xC3] = apduh_unk, [0xC4] = apduh_unk, [0xC5] = apduh_unk,
    [0xC6] = apduh_unk, [0xC7] = apduh_unk, [0xC8] = apduh_unk,
    [0xC9] = apduh_unk, [0xCA] = apduh_unk, [0xCB] = apduh_unk,
    [0xCC] = apduh_unk, [0xCD] = apduh_unk, [0xCE] = apduh_unk,
    [0xCF] = apduh_unk, [0xD0] = apduh_unk, [0xD1] = apduh_unk,
    [0xD2] = apduh_unk, [0xD3] = apduh_unk, [0xD4] = apduh_unk,
    [0xD5] = apduh_unk, [0xD6] = apduh_unk, [0xD7] = apduh_unk,
    [0xD8] = apduh_unk, [0xD9] = apduh_unk, [0xDA] = apduh_unk,
    [0xDB] = apduh_unk, [0xDC] = apduh_unk, [0xDD] = apduh_unk,
    [0xDE] = apduh_unk, [0xDF] = apduh_unk, [0xE0] = apduh_unk,
    [0xE1] = apduh_unk, [0xE2] = apduh_unk, [0xE3] = apduh_unk,
    [0xE4] = apduh_unk, [0xE5] = apduh_unk, [0xE6] = apduh_unk,
    [0xE7] = apduh_unk, [0xE8] = apduh_unk, [0xE9] = apduh_unk,
    [0xEA] = apduh_unk, [0xEB] = apduh_unk, [0xEC] = apduh_unk,
    [0xED] = apduh_unk, [0xEE] = apduh_unk, [0xEF] = apduh_unk,
    [0xF0] = apduh_unk, [0xF1] = apduh_unk, [0xF2] = apduh_unk,
    [0xF3] = apduh_unk, [0xF4] = apduh_unk, [0xF5] = apduh_unk,
    [0xF6] = apduh_unk, [0xF7] = apduh_unk, [0xF8] = apduh_unk,
    [0xF9] = apduh_unk, [0xFA] = apduh_unk, [0xFB] = apduh_unk,
    [0xFC] = apduh_unk, [0xFD] = apduh_unk, [0xFE] = apduh_unk,
    [0xFF] = apduh_unk,
};
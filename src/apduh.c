#include "uicc.h"
#include <byteswap.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

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
 * @todo Handle special (reserved) IDs.
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
    if (lc != cmd->data->len || (cmd->hdr->p2 & 0b11110000) != 0)
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

    uicc_fs_occ_et occ;

    enum data_req_e
    {
        DATA_REQ_RFU,

        DATA_REQ_FCI,    /* Return FCI template. Optional use of FCI tag and
                            length. */
        DATA_REQ_FCP,    /* Return FCP template. Mandatory use of FCP tag and
                          * length.
                          */
        DATA_REQ_FMD,    /* Return FMD template. Mandatory use of FMD tag and
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
        switch (cmd->hdr->p2 & 0b00000011)
        {
        case 0b00000000:
            occ = UICC_FS_OCC_FIRST;
            break;
        case 0b00000001:
            occ = UICC_FS_OCC_LAST;
            break;
        case 0b00000010:
            occ = UICC_FS_OCC_NEXT;
            break;
        case 0b00000011:
            occ = UICC_FS_OCC_PREV;
            break;
        }
        switch (cmd->hdr->p2 & 0b00001100)
        {
        case 0b00000000:
            data_req = DATA_REQ_FCI;
            break;
        case 0b00000100:
            data_req = DATA_REQ_FCP;
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
        if (meth == METH_RFU || data_req == DATA_REQ_RFU || meth == METH_DO ||
            meth == METH_DO_PARENT)
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
                /* Check if maybe trying to select an ADF. */
                if (cmd->data->len > UICC_FS_ADF_AID_LEN ||
                    cmd->data->len < UICC_FS_ADF_AID_RID_LEN)
                {
                    ret_select = UICC_RET_ERROR;
                }
                else
                {
                    ret_select = uicc_va_select_adf(
                        &uicc_state->internal.fs, cmd->data->b,
                        cmd->data->len - UICC_FS_ADF_AID_RID_LEN);
                }
            }
            else
            {
                ret_select = uicc_va_select_file_id(
                    &uicc_state->internal.fs, *(uicc_fs_id_kt *)cmd->data->b);
            }
            break;
        case METH_DF_NESTED:
        case METH_EF_NESTED:
        case METH_DF_PARENT:
            ret_select = UICC_RET_ERROR;
            break;
        case METH_DF_NAME:
            /* Name must be at least 1 char long. */
            if (cmd->data->len == 0 || occ != UICC_FS_OCC_FIRST)
            {
                ret_select = UICC_RET_ERROR;
            }
            else
            {
                ret_select = uicc_va_select_file_dfname(
                    &uicc_state->internal.fs, (char *)cmd->data->b,
                    cmd->data->len);
            }
            break;
        case METH_MF_PATH:
            /* Must contain at least 1 ID in the path. */
            if (cmd->data->len < sizeof(uicc_fs_id_kt) ||
                occ != UICC_FS_OCC_FIRST)
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
                ret_select =
                    uicc_va_select_file_path(&uicc_state->internal.fs, path);
            }
            break;
        case METH_DF_PATH:
            /* Must contain at least 1 ID in the path. */
            if (cmd->data->len < sizeof(uicc_fs_id_kt) ||
                occ != UICC_FS_OCC_FIRST)
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
                ret_select =
                    uicc_va_select_file_path(&uicc_state->internal.fs, path);
            }
            break;
        default:
            /* Unreachable due to the parameter rejection done before. */
            __builtin_unreachable();
        }

        if (ret_select == UICC_RET_FS_NOT_FOUND)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x82; /* "Not found" */
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }
        else if (ret_select != UICC_RET_SUCCESS)
        {
            /* Failed to select. */
            res->sw1 = UICC_APDU_SW1_CHER_UNK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        uicc_fs_file_hdr_st *file_selected = NULL;
        bool file_selected_type_folder = false;
        if (uicc_state->internal.fs.va.cur_ef.item.type !=
            UICC_FS_ITEM_TYPE_INVALID)
        {
            file_selected = &uicc_state->internal.fs.va.cur_ef;
        }
        else if (uicc_state->internal.fs.va.cur_df.item.type !=
                 UICC_FS_ITEM_TYPE_INVALID)
        {
            file_selected_type_folder = true;
            file_selected = &uicc_state->internal.fs.va.cur_df;
        }
        else
        {
            /* No file was actually selected. */
            res->sw1 = UICC_APDU_SW1_CHER_UNK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        if (data_req == DATA_REQ_ABSENT)
        {
            res->sw1 = UICC_APDU_SW1_NORM_NONE;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }
        else
        {
            /**
             * Create tags for use in encoding.
             * ISO 7816-4:2020 p.27 sec.7.4.3 table.11.
             */
            static uint8_t const tags[] = {
                0x62, /* FCP Template */
                0x64, /* FMD Template */
                0x6F, /* FCI Template */
                0x80, /* Data byte count */
                0x82, /* File descripotor and coding */
                0x83, /* File ID */
                0x84, /* DF Name */
                0x88, /* Short File ID */
                0x8A, /* Life cycle status */
            };
            static uint32_t const tags_count = sizeof(tags) / sizeof(tags[0U]);
            uicc_dato_bertlv_tag_st bertlv_tags[tags_count];
            for (uint8_t tag_idx = 0U; tag_idx < tags_count; ++tag_idx)
            {
                if (uicc_dato_bertlv_tag_create(&bertlv_tags[tag_idx],
                                                tags[tag_idx]) !=
                    UICC_RET_SUCCESS)
                {
                    res->sw1 = UICC_APDU_SW1_CHER_UNK;
                    res->sw2 = 0U;
                    res->data.len = 0U;
                    return UICC_RET_SUCCESS;
                }
            }

            /* Create data for BER-TLV DOs. */
            /**
             * Safe cast since size will at least 0 since size includes
             * the header length.
             */
            uint32_t const data_size =
                (uint32_t)(file_selected->item.size -
                           sizeof(uicc_fs_file_hdr_raw_st));
            uint8_t const data_size_be[] = {
                (uint8_t)((data_size & 0xFF000000) >> (3U * 8U)),
                (uint8_t)((data_size & 0x00FF0000) >> (2U * 8U)),
                (uint8_t)((data_size & 0x0000FF00) >> (1U * 8U)),
                (uint8_t)((data_size & 0x000000FF) >> (0U * 8U)),
            };
            uint8_t const data_id[] = {
                (uint8_t)((file_selected->id & 0xFF00) >> 8U),
                (uint8_t)(file_selected->id & 0x00FF),
            };
            uint8_t const data_sid[] = {file_selected->sid};
            uint8_t lcs_be[1U];
            uint8_t desc_be[2U];
            if (uicc_file_lcs(file_selected, &lcs_be[0U]) != UICC_RET_SUCCESS ||
                uicc_file_descr(file_selected, &desc_be[0U]) !=
                    UICC_RET_SUCCESS ||
                uicc_file_data_coding(file_selected, &desc_be[1U]) !=
                    UICC_RET_SUCCESS)
            {
                res->sw1 = UICC_APDU_SW1_CHER_UNK;
                res->sw2 = 0U;
                res->data.len = 0U;
                return UICC_RET_SUCCESS;
            }

            uint8_t *bertlv_buf;
            uint32_t bertlv_len;
            uicc_ret_et ret_bertlv = UICC_RET_ERROR;
            uicc_dato_bertlv_enc_st enc;
            for (bool dry_run = true;; dry_run = false)
            {
                if (dry_run)
                {
                    bertlv_buf = NULL;
                    bertlv_len = 0;
                }
                else
                {
                    bertlv_buf = res->data.b;
                    if (enc.len > sizeof(res->data.b))
                    {
                        break;
                    }
                    bertlv_len = enc.len;
                }

                uicc_dato_bertlv_enc_init(&enc, bertlv_buf, bertlv_len);
                uicc_dato_bertlv_enc_st enc_nstd;

                /* Nest everything in an FCI if it was requested. */
                if (data_req == DATA_REQ_FCI)
                {
                    if (uicc_dato_bertlv_enc_nstd_start(&enc, &enc_nstd) !=
                        UICC_RET_SUCCESS)
                    {
                        break;
                    }
                }
                else
                {
                    /**
                     * If there is no FCI, nested encoder is just the root
                     * encoder.
                     */
                    enc_nstd = enc;
                }
                /* Create an FCP if it was requested. */
                if (data_req == DATA_REQ_FCI || data_req == DATA_REQ_FCP)
                {
                    uicc_dato_bertlv_enc_st enc_fcp;
                    if (uicc_dato_bertlv_enc_nstd_start(&enc_nstd, &enc_fcp) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_data(
                            &enc_fcp, data_size_be,
                            sizeof(data_size_be) / sizeof(data_size_be[0U])) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_hdr(&enc_fcp, &bertlv_tags[3U]) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_data(&enc_fcp, desc_be,
                                                  sizeof(desc_be) /
                                                      sizeof(desc_be[0U])) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_hdr(&enc_fcp, &bertlv_tags[4U]) !=
                            UICC_RET_SUCCESS ||
                        (file_selected->id != 0
                             ? uicc_dato_bertlv_enc_data(
                                   &enc_fcp, data_id,
                                   sizeof(data_id) / sizeof(data_id[0U])) !=
                                       UICC_RET_SUCCESS ||
                                   uicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                            &bertlv_tags[5U]) !=
                                       UICC_RET_SUCCESS
                             : false) ||
                        (file_selected_type_folder
                             ? uicc_dato_bertlv_enc_data(
                                   &enc_fcp, (uint8_t *)file_selected->name,
                                   UICC_FS_NAME_LEN_MAX) != UICC_RET_SUCCESS ||
                                   uicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                            &bertlv_tags[6U]) !=
                                       UICC_RET_SUCCESS
                             : false) ||
                        (!file_selected_type_folder && file_selected->sid != 0
                             ? uicc_dato_bertlv_enc_data(
                                   &enc_fcp, data_sid,
                                   sizeof(data_sid) / sizeof(data_sid[0U])) !=
                                       UICC_RET_SUCCESS ||
                                   uicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                            &bertlv_tags[7U]) !=
                                       UICC_RET_SUCCESS
                             : false) ||
                        uicc_dato_bertlv_enc_data(&enc_fcp, lcs_be,
                                                  sizeof(lcs_be) /
                                                      sizeof(lcs_be[0U])) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_hdr(&enc_fcp, &bertlv_tags[8U]) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_nstd_end(&enc_nstd, &enc_fcp) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_hdr(&enc_nstd, &bertlv_tags[0U]) !=
                            UICC_RET_SUCCESS)
                    {
                        break;
                    }
                }
                /* Create an FMD if it was requested. */
                if (data_req == DATA_REQ_FCI || data_req == DATA_REQ_FMD)
                {
                    uicc_dato_bertlv_enc_st enc_fmd;
                    if (uicc_dato_bertlv_enc_nstd_start(&enc_nstd, &enc_fmd) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_nstd_end(&enc_nstd, &enc_fmd) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_hdr(&enc_nstd, &bertlv_tags[1U]) !=
                            UICC_RET_SUCCESS)
                    {
                        break;
                    }
                }
                if (data_req == DATA_REQ_FCI)
                {
                    if (uicc_dato_bertlv_enc_nstd_end(&enc, &enc_nstd) !=
                            UICC_RET_SUCCESS ||
                        uicc_dato_bertlv_enc_hdr(&enc, &bertlv_tags[2U]) !=
                            UICC_RET_SUCCESS)
                    {
                        break;
                    }
                }
                else
                {
                    /* Write back to the main encoder. */
                    enc = enc_nstd;
                }

                /* Stop when finished with the real run (i.e. not dry run). */
                if (!dry_run)
                {
                    ret_bertlv = UICC_RET_SUCCESS;
                    break;
                }
            }
            if (ret_bertlv == UICC_RET_SUCCESS)
            {
                res->sw1 = UICC_APDU_SW1_NORM_NONE;
                res->sw2 = 0U;
                /* Safe cast due check inside the BER-TLV loop. */
                res->data.len = (uint16_t)bertlv_len;
                return UICC_RET_SUCCESS;
            }
            else
            {
                res->sw1 = UICC_APDU_SW1_CHER_UNK;
                res->sw2 = 0U;
                res->data.len = 0U;
                return UICC_RET_SUCCESS;
            }
        }
    }
}

/**
 * @brief Handle the READ BINARY command in the interindustry class.
 * @param uicc_state
 * @param cmd
 * @param res
 * @return Return code.
 * @note As described in ISO 7816-4:2020 p.74 sec.11.3.3.
 */
static uicc_apduh_ft apduh_bin_read;
static uicc_ret_et apduh_bin_read(uicc_st *const uicc_state,
                                  uicc_apdu_cmd_st const *const cmd,
                                  uicc_apdu_res_st *const res)
{
    /**
     * Odd instruction (B1) not supported. It would have the data field encoded
     * as a BER-TLV DO.
     */
    if (cmd->hdr->ins != 0xB0)
    {
        res->sw1 = UICC_APDU_SW1_CHER_INS;
        res->sw2 = 0U;
        res->data.len = 0U;
        return UICC_RET_SUCCESS;
    }

    uint8_t const len_expected = *cmd->p3;
    uint16_t offset;
    uicc_fs_file_hdr_st file;

    /**
     * Indicates if P1 contains a SID thus leading to a lookup and change in
     * current EF in VA on successful read.
     */
    bool const sid_use = cmd->hdr->p1 & 0b10000000;
    uicc_fs_sid_kt sid;

    /**
     * Parse P1 and P2.
     * @note The standard refers to b1 of INS which essentially differentiates
     * between the even B0 and odd B1 instructions. The latter is not supported.
     */
    if (sid_use)
    {
        /**
         * b7 and b6 of P1 must be set to 00, b5 to b1 of P1 encodes SFI and P2
         * encodes an offset from 0 to 255 in the EF referenced by command.
         */
        /* b6 and b7 of P1 must be 0. */
        if ((cmd->hdr->p1 & 0b01100000) != 0)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        sid = cmd->hdr->p1 & 0b00011111;
        offset = cmd->hdr->p2;

        uicc_ret_et const ret_lookup = uicc_disk_lutsid_lookup(
            uicc_state->internal.fs.va.cur_adf, sid, &file);
        if (ret_lookup == UICC_RET_FS_NOT_FOUND)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x82; /* "File or application not found" */
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }
        else if (ret_lookup != UICC_RET_SUCCESS)
        {
            /* Not sure what went wrong. */
            res->sw1 = UICC_APDU_SW1_CHER_UNK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }
    }
    else
    {
        /**
         * P1-P2 (15 bits) encodes an offset in the EF
         * referenced by curEF from 0 to 32767.
         */
        /* Safe cast since just concatentating 2 bytes into short. */
        offset = (uint16_t)(((0b01111111 & cmd->hdr->p1) << 8U) | cmd->hdr->p2);

        file = uicc_state->internal.fs.va.cur_ef;
        if (file.item.type == UICC_FS_ITEM_TYPE_INVALID)
        {
            res->sw1 = UICC_APDU_SW1_CHER_CMD;
            res->sw2 = 0x86; /* "Command not allowed (curEF not set)" */
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }
    }

    if (file.item.type == UICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT)
    {
        if (offset >= file.item.size - sizeof(uicc_fs_file_hdr_raw_st))
        {
            /**
             * Requested an offset which is outside the bounds of the
             * file.
             */
            res->sw1 = UICC_APDU_SW1_CHER_P1P2;
            res->sw2 = 0U;
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        /* Read data into response. */
        /* Safe cast due to the check against offset. */
        uint8_t const len_readable =
            (uint8_t)(file.item.size - sizeof(uicc_fs_file_hdr_raw_st) -
                      offset);
        uint8_t const len_read =
            len_expected > len_readable ? len_readable : len_expected;
        memcpy(res->data.b,
               &uicc_state->internal.fs.va.cur_adf
                    ->buf[file.item.offset_trel +
                          sizeof(uicc_fs_file_hdr_raw_st) + offset],
               len_read);
        res->data.len = len_read;
        if (len_read < len_expected)
        {
            /* Read fewer bytes than were requested. */
            res->sw1 = UICC_APDU_SW1_WARN_NVM_CHGN;
            res->sw2 = 0x82; /* "End of file, record or DO reached before
                                reading Ne bytes, or unsuccessful search" */
        }
        else
        {
            /* Read exactly the number of bytes requested. */
            res->sw1 = UICC_APDU_SW1_NORM_NONE;
            res->sw2 = 0U;
        }

        if (sid_use)
        {
            /**
             * Select the file by SID now that the command is known to
             * succeed.
             */
            if (uicc_va_select_file_sid(&uicc_state->internal.fs, sid) ==
                UICC_RET_SUCCESS)
            {
                return UICC_RET_SUCCESS;
            }
            /**
             * Selection should not fail since the lookup worked just fine.
             * This will fall-through to the unknown error.
             */
        }
        else
        {
            /* Nothing extra to be done on simple read by offset. */
            return UICC_RET_SUCCESS;
        }
    }
    else
    {
        res->sw1 = UICC_APDU_SW1_CHER_CMD;
        res->sw2 = 0x81; /* "Command incompatible with file structure" */
        res->data.len = 0U;
        return UICC_RET_SUCCESS;
    }

    res->sw1 = UICC_APDU_SW1_CHER_UNK;
    res->sw2 = 0U;
    res->data.len = 0U;
    return UICC_RET_SUCCESS;
}

/**
 * @brief Handle the READ RECORD command in the interindustry class.
 * @param uicc_state
 * @param cmd
 * @param res
 * @return Return code.
 * @note As described in ISO 7816-4:2020 p.82 sec.11.4.3.
 */
static uicc_apduh_ft apduh_rcrd_read;
static uicc_ret_et apduh_rcrd_read(uicc_st *const uicc_state,
                                   uicc_apdu_cmd_st const *const cmd,
                                   uicc_apdu_res_st *const res)
{
    /**
     * Odd instruction (B3) not supported. It would have the data field encoded
     * as a BER-TLV DO.
     */
    if (cmd->hdr->ins != 0xB2)
    {
        res->sw1 = UICC_APDU_SW1_CHER_INS;
        res->sw2 = 0U;
        res->data.len = 0U;
        return UICC_RET_SUCCESS;
    }

    /* Where to read records from. */
    enum trgt_e
    {
        TRGT_EF_CUR,
        TRGT_EF_SID,
        TRGT_MANY,
    } trgt;

    /* Which occurrence to read when reading using an ID. */
    __attribute__((unused)) uicc_fs_occ_et occ = UICC_FS_OCC_FIRST;

    /* What to record(s) to read when reading using a number. */
    enum what_e
    {
        WHAT_P1,
        WHAT_P1_TO_LAST,
        WHAT_LAST_TO_P1,
        WHAT_RFU,
    } what = WHAT_RFU;

    /* Method of selecting a record to read. */
    enum meth_e
    {
        METH_RCRD_ID,
        METH_RCRD_NUM,
    } meth;

    /**
     * Value of first 5 bits. This can be interpreted differently based on the
     * other bits and P1.
     */
    uint8_t const p2_val = (cmd->hdr->p2 & 0b11111000) >> 3U;

    /* Parse P1 and P2 */
    {
        switch (p2_val)
        {
        case 0b00000:
            trgt = TRGT_EF_CUR;
            break;
        case 0b11111:
            trgt = TRGT_MANY;
            break;
        default:
            trgt = TRGT_EF_SID;
            break;
        }

        if (cmd->hdr->p2 & 0b00000100)
        {
            meth = METH_RCRD_NUM;
            switch (cmd->hdr->p2 & 0b00000011)
            {
            case 0b00:
                what = WHAT_P1;
                break;
            case 0b01:
                what = WHAT_P1_TO_LAST;
                break;
            case 0b10:
                what = WHAT_LAST_TO_P1;
                break;
            default:
                what = WHAT_RFU;
                break;
            }
        }
        else
        {
            meth = METH_RCRD_ID;
            switch (cmd->hdr->p2 & 0b00000011)
            {
            case 0b00:
                occ = UICC_FS_OCC_FIRST;
                break;
            case 0b01:
                occ = UICC_FS_OCC_LAST;
                break;
            case 0b10:
                occ = UICC_FS_OCC_NEXT;
                break;
            case 0b11:
                occ = UICC_FS_OCC_PREV;
                break;
            }
        }
    }

    /* Perform the requested operation. */
    {
        /**
         * Operation "P1 set to '00' and one or more record handling
         * DO'7F76' in the command data field", selection by ID is not
         * supported, reading many records is not supported.
         */
        if (cmd->hdr->p2 == 0b11111000 || meth == METH_RCRD_ID ||
            trgt == TRGT_MANY)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x81; /* "Function not supported" */
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        /**
         * RFU values should never be received.
         * P1 = 0x00 is used for "special purposes" and P1 = 0xFF is RFU per
         * ISO 7816-4:2020 p.82 sec.11.4.2.
         */
        if ((p2_val == 0b11111 && meth == METH_RCRD_ID) ||
            (p2_val == 0b11111 && meth == METH_RCRD_NUM) || what == WHAT_RFU ||
            cmd->hdr->p1 == 0x00 || cmd->hdr->p1 == 0xFF)
        {
            res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
            res->data.len = 0U;
            return UICC_RET_SUCCESS;
        }

        if (meth == METH_RCRD_NUM)
        {
            /* Safe cast because P1 is >0. */
            uicc_fs_rcrd_idx_kt const rcrd_idx = (uint8_t)(cmd->hdr->p1 - 1U);

            uicc_fs_file_hdr_st ef_cur;
            uicc_ret_et ret_ef = UICC_RET_ERROR;
            switch (trgt)
            {
            case TRGT_EF_CUR: {
                ef_cur = uicc_state->internal.fs.va.cur_ef;
                ret_ef = UICC_RET_SUCCESS;
                break;
            }
            case TRGT_EF_SID: {
                uicc_fs_sid_kt const sid = p2_val;
                ret_ef = uicc_disk_lutsid_lookup(
                    uicc_state->internal.fs.va.cur_adf, sid, &ef_cur);
                break;
            }
            default:
                /* Already rejected 'many' operation before. */
                __builtin_unreachable();
            }

            if (ret_ef == UICC_RET_FS_NOT_FOUND)
            {
                res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
                res->sw2 = 0x82; /* "File or application not found" */
                res->data.len = 0U;
                return UICC_RET_SUCCESS;
            }
            else if (ret_ef == UICC_RET_SUCCESS)
            {
                /* Got the target EF, can read the record now. */
                uint8_t *rcrd_buf;
                uint8_t rcrd_len;
                uicc_ret_et const ret_rcrd = uicc_disk_file_rcrd(
                    uicc_state->internal.fs.va.cur_adf, &ef_cur, rcrd_idx,
                    &rcrd_buf, &rcrd_len);
                if (ret_rcrd == UICC_RET_FS_NOT_FOUND)
                {
                    res->sw1 = UICC_APDU_SW1_CHER_P1P2_INFO;
                    res->sw2 = 0x83; /* "Record not found" */
                    res->data.len = 0U;
                    return UICC_RET_SUCCESS;
                }
                else if (ret_rcrd == UICC_RET_SUCCESS)
                {
                    /* Check if the expected response length is as expected. */
                    if (*cmd->p3 != rcrd_len)
                    {
                        /**
                         * Asking the interface to retry the command but this
                         * time with correct expected response length.
                         */
                        res->sw1 = UICC_APDU_SW1_CHER_LE;
                        res->sw2 = rcrd_len;
                        res->data.len = 0U;
                        return UICC_RET_SUCCESS;
                    }

                    /**
                     * Have to select the file on success (only if EF was
                     * selected by SID).
                     * @warning If this fails, something weird is going on.
                     */
                    uicc_ret_et const ret_file_select =
                        trgt == TRGT_EF_SID
                            ? uicc_va_select_file_sid(&uicc_state->internal.fs,
                                                      ef_cur.sid)
                            : UICC_RET_SUCCESS;
                    if (ret_file_select == UICC_RET_SUCCESS)
                    {
                        /**
                         * In any case, have to select the record.
                         * @warning If this fails, something weird is going on.
                         */
                        uicc_ret_et const ret_rcrd_select =
                            uicc_va_select_record_idx(&uicc_state->internal.fs,
                                                      rcrd_idx);
                        if (ret_rcrd_select == UICC_RET_SUCCESS)
                        {
                            res->sw1 = UICC_APDU_SW1_NORM_NONE;
                            res->sw2 = 0U;
                            res->data.len = rcrd_len;
                            memcpy(res->data.b, rcrd_buf, rcrd_len);
                            return UICC_RET_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    res->sw1 = UICC_APDU_SW1_CHER_UNK;
    res->sw2 = 0U;
    res->data.len = 0U;
    return UICC_RET_SUCCESS;
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
    [0x00] = apduh_unk,      [0x01] = apduh_unk,       [0x02] = apduh_unk,
    [0x03] = apduh_unk,      [0x04] = apduh_unk,       [0x05] = apduh_unk,
    [0x06] = apduh_unk,      [0x07] = apduh_unk,       [0x08] = apduh_unk,
    [0x09] = apduh_unk,      [0x0A] = apduh_unk,       [0x0B] = apduh_unk,
    [0x0C] = apduh_unk,      [0x0D] = apduh_unk,       [0x0E] = apduh_unk,
    [0x0F] = apduh_unk,      [0x10] = apduh_unk,       [0x11] = apduh_unk,
    [0x12] = apduh_unk,      [0x13] = apduh_unk,       [0x14] = apduh_unk,
    [0x15] = apduh_unk,      [0x16] = apduh_unk,       [0x17] = apduh_unk,
    [0x18] = apduh_unk,      [0x19] = apduh_unk,       [0x1A] = apduh_unk,
    [0x1B] = apduh_unk,      [0x1C] = apduh_unk,       [0x1D] = apduh_unk,
    [0x1E] = apduh_unk,      [0x1F] = apduh_unk,       [0x20] = apduh_unk,
    [0x21] = apduh_unk,      [0x22] = apduh_unk,       [0x23] = apduh_unk,
    [0x24] = apduh_unk,      [0x25] = apduh_unk,       [0x26] = apduh_unk,
    [0x27] = apduh_unk,      [0x28] = apduh_unk,       [0x29] = apduh_unk,
    [0x2A] = apduh_unk,      [0x2B] = apduh_unk,       [0x2C] = apduh_unk,
    [0x2D] = apduh_unk,      [0x2E] = apduh_unk,       [0x2F] = apduh_unk,
    [0x30] = apduh_unk,      [0x31] = apduh_unk,       [0x32] = apduh_unk,
    [0x33] = apduh_unk,      [0x34] = apduh_unk,       [0x35] = apduh_unk,
    [0x36] = apduh_unk,      [0x37] = apduh_unk,       [0x38] = apduh_unk,
    [0x39] = apduh_unk,      [0x3A] = apduh_unk,       [0x3B] = apduh_unk,
    [0x3C] = apduh_unk,      [0x3D] = apduh_unk,       [0x3E] = apduh_unk,
    [0x3F] = apduh_unk,      [0x40] = apduh_unk,       [0x41] = apduh_unk,
    [0x42] = apduh_unk,      [0x43] = apduh_unk,       [0x44] = apduh_unk,
    [0x45] = apduh_unk,      [0x46] = apduh_unk,       [0x47] = apduh_unk,
    [0x48] = apduh_unk,      [0x49] = apduh_unk,       [0x4A] = apduh_unk,
    [0x4B] = apduh_unk,      [0x4C] = apduh_unk,       [0x4D] = apduh_unk,
    [0x4E] = apduh_unk,      [0x4F] = apduh_unk,       [0x50] = apduh_unk,
    [0x51] = apduh_unk,      [0x52] = apduh_unk,       [0x53] = apduh_unk,
    [0x54] = apduh_unk,      [0x55] = apduh_unk,       [0x56] = apduh_unk,
    [0x57] = apduh_unk,      [0x58] = apduh_unk,       [0x59] = apduh_unk,
    [0x5A] = apduh_unk,      [0x5B] = apduh_unk,       [0x5C] = apduh_unk,
    [0x5D] = apduh_unk,      [0x5E] = apduh_unk,       [0x5F] = apduh_unk,
    [0x60] = apduh_unk,      [0x61] = apduh_unk,       [0x62] = apduh_unk,
    [0x63] = apduh_unk,      [0x64] = apduh_unk,       [0x65] = apduh_unk,
    [0x66] = apduh_unk,      [0x67] = apduh_unk,       [0x68] = apduh_unk,
    [0x69] = apduh_unk,      [0x6A] = apduh_unk,       [0x6B] = apduh_unk,
    [0x6C] = apduh_unk,      [0x6D] = apduh_unk,       [0x6E] = apduh_unk,
    [0x6F] = apduh_unk,      [0x70] = apduh_unk,       [0x71] = apduh_unk,
    [0x72] = apduh_unk,      [0x73] = apduh_unk,       [0x74] = apduh_unk,
    [0x75] = apduh_unk,      [0x76] = apduh_unk,       [0x77] = apduh_unk,
    [0x78] = apduh_unk,      [0x79] = apduh_unk,       [0x7A] = apduh_unk,
    [0x7B] = apduh_unk,      [0x7C] = apduh_unk,       [0x7D] = apduh_unk,
    [0x7E] = apduh_unk,      [0x7F] = apduh_unk,       [0x80] = apduh_unk,
    [0x81] = apduh_unk,      [0x82] = apduh_unk,       [0x83] = apduh_unk,
    [0x84] = apduh_unk,      [0x85] = apduh_unk,       [0x86] = apduh_unk,
    [0x87] = apduh_unk,      [0x88] = apduh_unk,       [0x89] = apduh_unk,
    [0x8A] = apduh_unk,      [0x8B] = apduh_unk,       [0x8C] = apduh_unk,
    [0x8D] = apduh_unk,      [0x8E] = apduh_unk,       [0x8F] = apduh_unk,
    [0x90] = apduh_unk,      [0x91] = apduh_unk,       [0x92] = apduh_unk,
    [0x93] = apduh_unk,      [0x94] = apduh_unk,       [0x95] = apduh_unk,
    [0x96] = apduh_unk,      [0x97] = apduh_unk,       [0x98] = apduh_unk,
    [0x99] = apduh_unk,      [0x9A] = apduh_unk,       [0x9B] = apduh_unk,
    [0x9C] = apduh_unk,      [0x9D] = apduh_unk,       [0x9E] = apduh_unk,
    [0x9F] = apduh_unk,      [0xA0] = apduh_unk,       [0xA1] = apduh_unk,
    [0xA2] = apduh_unk,      [0xA3] = apduh_unk,       [0xA4] = apduh_select,
    [0xA5] = apduh_unk,      [0xA6] = apduh_unk,       [0xA7] = apduh_unk,
    [0xA8] = apduh_unk,      [0xA9] = apduh_unk,       [0xAA] = apduh_unk,
    [0xAB] = apduh_unk,      [0xAC] = apduh_unk,       [0xAD] = apduh_unk,
    [0xAE] = apduh_unk,      [0xAF] = apduh_unk,       [0xB0] = apduh_bin_read,
    [0xB1] = apduh_bin_read, [0xB2] = apduh_rcrd_read, [0xB3] = apduh_rcrd_read,
    [0xB4] = apduh_unk,      [0xB5] = apduh_unk,       [0xB6] = apduh_unk,
    [0xB7] = apduh_unk,      [0xB8] = apduh_unk,       [0xB9] = apduh_unk,
    [0xBA] = apduh_unk,      [0xBB] = apduh_unk,       [0xBC] = apduh_unk,
    [0xBD] = apduh_unk,      [0xBE] = apduh_unk,       [0xBF] = apduh_unk,
    [0xC0] = apduh_unk,      [0xC1] = apduh_unk,       [0xC2] = apduh_unk,
    [0xC3] = apduh_unk,      [0xC4] = apduh_unk,       [0xC5] = apduh_unk,
    [0xC6] = apduh_unk,      [0xC7] = apduh_unk,       [0xC8] = apduh_unk,
    [0xC9] = apduh_unk,      [0xCA] = apduh_unk,       [0xCB] = apduh_unk,
    [0xCC] = apduh_unk,      [0xCD] = apduh_unk,       [0xCE] = apduh_unk,
    [0xCF] = apduh_unk,      [0xD0] = apduh_unk,       [0xD1] = apduh_unk,
    [0xD2] = apduh_unk,      [0xD3] = apduh_unk,       [0xD4] = apduh_unk,
    [0xD5] = apduh_unk,      [0xD6] = apduh_unk,       [0xD7] = apduh_unk,
    [0xD8] = apduh_unk,      [0xD9] = apduh_unk,       [0xDA] = apduh_unk,
    [0xDB] = apduh_unk,      [0xDC] = apduh_unk,       [0xDD] = apduh_unk,
    [0xDE] = apduh_unk,      [0xDF] = apduh_unk,       [0xE0] = apduh_unk,
    [0xE1] = apduh_unk,      [0xE2] = apduh_unk,       [0xE3] = apduh_unk,
    [0xE4] = apduh_unk,      [0xE5] = apduh_unk,       [0xE6] = apduh_unk,
    [0xE7] = apduh_unk,      [0xE8] = apduh_unk,       [0xE9] = apduh_unk,
    [0xEA] = apduh_unk,      [0xEB] = apduh_unk,       [0xEC] = apduh_unk,
    [0xED] = apduh_unk,      [0xEE] = apduh_unk,       [0xEF] = apduh_unk,
    [0xF0] = apduh_unk,      [0xF1] = apduh_unk,       [0xF2] = apduh_unk,
    [0xF3] = apduh_unk,      [0xF4] = apduh_unk,       [0xF5] = apduh_unk,
    [0xF6] = apduh_unk,      [0xF7] = apduh_unk,       [0xF8] = apduh_unk,
    [0xF9] = apduh_unk,      [0xFA] = apduh_unk,       [0xFB] = apduh_unk,
    [0xFC] = apduh_unk,      [0xFD] = apduh_unk,       [0xFE] = apduh_unk,
    [0xFF] = apduh_unk,
};

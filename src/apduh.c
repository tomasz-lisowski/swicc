#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <swicc/swicc.h>

/**
 * @brief Handle both invalid and unknown instructions.
 */
static swicc_apduh_ft apduh_unk;
static swicc_ret_et apduh_unk(swicc_st *const swicc_state,
                              swicc_apdu_cmd_st const *const cmd,
                              swicc_apdu_res_st *const res,
                              uint32_t const procedure_count)
{
    res->sw1 = SWICC_APDU_SW1_CHER_INS;
    res->sw2 = 0;
    res->data.len = 0;
    return SWICC_RET_SUCCESS;
}

/**
 * @brief Handle the SELECT command in the interindustry class.
 * @note As described in ISO/IEC 7816-4:2020 p.74 sec.11.2.2.
 */
static swicc_apduh_ft apduh_select;
static swicc_ret_et apduh_select(swicc_st *const swicc_state,
                                 swicc_apdu_cmd_st const *const cmd,
                                 swicc_apdu_res_st *const res,
                                 uint32_t const procedure_count)
{
    /**
     * ISO/IEC 7816-4:2020 pg.75 sec.11.2.2 table.63 states any value with not
     * all 0's at start is RFU.
     */
    if ((cmd->hdr->p2 & 0b11110000) != 0)
    {
        res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
        res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    /**
     * Check if we only got Lc which means we need to send back a procedure
     * byte.
     */
    if (procedure_count == 0U)
    {
        /**
         * Unexpected because before sending a procedure, no data should have
         * been received.
         */
        if (cmd->data->len != 0U)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_UNK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        /**
         * If Lc is 0 it means data is absent so we can process what we got,
         * otherwise we need more from the interface.
         */
        if (*cmd->p3 > 0)
        {
            res->sw1 = SWICC_APDU_SW1_PROC_ACK_ALL;
            res->sw2 = 0U;
            res->data.len = *cmd->p3; /* Length of expected data. */
            return SWICC_RET_SUCCESS;
        }
    }

    /**
     * The ACK ALL procedure was sent and we expected to receive all the data
     * (length of which was given in P3) but did not receive the expected amount
     * of data.
     */
    if (cmd->data->len != *cmd->p3 && procedure_count >= 1U)
    {
        res->sw1 = SWICC_APDU_SW1_CHER_LEN;
        res->sw2 = 0x02; /* The value of Lc is not the one expected. */
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    enum meth_e
    {
        METH_RFU,

        METH_MF_ADF_DF_EF, /* Select MF, DF, or EF. Data: file ID or absent.
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

    swicc_fs_occ_et occ;

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
            meth = METH_MF_ADF_DF_EF;
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
            occ = SWICC_FS_OCC_FIRST;
            break;
        case 0b00000001:
            occ = SWICC_FS_OCC_LAST;
            break;
        case 0b00000010:
            occ = SWICC_FS_OCC_NEXT;
            break;
        case 0b00000011:
            occ = SWICC_FS_OCC_PREV;
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
            meth == METH_DO_PARENT || meth == METH_DF_PARENT ||
            meth == METH_EF_NESTED || meth == METH_DF_NESTED)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2;
            res->sw2 = 0U;
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        swicc_ret_et ret_select = SWICC_RET_ERROR;
        switch (meth)
        {
        case METH_MF_ADF_DF_EF:
            /* Must contain exactly 1 file ID. */
            if (cmd->data->len != sizeof(swicc_fs_id_kt))
            {
                ret_select = SWICC_RET_ERROR;
            }
            else
            {
                swicc_fs_id_kt const fid = be16toh(*(uint16_t *)cmd->data->b);
                ret_select = swicc_va_select_file_id(&swicc_state->fs, fid);
            }
            break;
        case METH_DF_NAME:
            /* Check if maybe trying to select an ADF. */
            if (cmd->data->len > SWICC_FS_ADF_AID_LEN ||
                cmd->data->len < SWICC_FS_ADF_AID_RID_LEN ||
                occ != SWICC_FS_OCC_FIRST)
            {
                ret_select = SWICC_RET_ERROR;
            }
            else
            {
                /**
                 * Try selecting an ADF by name, if that fails fall back to
                 * selecting a DF by name.
                 */
                ret_select = swicc_va_select_adf(&swicc_state->fs, cmd->data->b,
                                                 cmd->data->len -
                                                     SWICC_FS_ADF_AID_RID_LEN);
                if (ret_select == SWICC_RET_FS_NOT_FOUND)
                {
                    ret_select = swicc_va_select_file_dfname(
                        &swicc_state->fs, cmd->data->b, cmd->data->len);
                }
            }
            break;
        case METH_MF_PATH:
        case METH_DF_PATH:
            /* Must contain at least 1 ID in the path. */
            if (cmd->data->len < sizeof(swicc_fs_id_kt) ||
                occ != SWICC_FS_OCC_FIRST || cmd->data->len % 2 != 0)
            {
                ret_select = SWICC_RET_ERROR;
            }
            else
            {
                swicc_fs_path_st const path = {
                    .b = (swicc_fs_id_kt *)cmd->data->b,
                    .len = cmd->data->len / 2,
                    .type = meth == METH_MF_PATH ? SWICC_FS_PATH_TYPE_MF
                                                 : SWICC_FS_PATH_TYPE_DF,
                };
                /* Convert path to host endianness. */
                for (uint32_t path_idx = 0U; path_idx < path.len; ++path_idx)
                {
                    path.b[path_idx] = be16toh(path.b[path_idx]);
                }
                ret_select = swicc_va_select_file_path(&swicc_state->fs, path);
            }
            break;
        default:
            /* Unreachable due to the parameter rejection done before. */
            __builtin_unreachable();
        }

        if (ret_select == SWICC_RET_FS_NOT_FOUND)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x82; /* "Not found" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
        else if (ret_select != SWICC_RET_SUCCESS)
        {
            /* Failed to select. */
            res->sw1 = SWICC_APDU_SW1_CHER_UNK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        /* The file that was requested to be selected. */
        swicc_fs_file_st *file_selected = &swicc_state->fs.va.cur_file;

        if (data_req == DATA_REQ_ABSENT)
        {
            res->sw1 = SWICC_APDU_SW1_NORM_NONE;
            res->sw2 = 0U;
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
        else
        {
            /**
             * Create tags for use in encoding.
             * ISO/IEC 7816-4:2020 p.27 sec.7.4.3 table.11.
             */
            static uint8_t const tags[] = {
                0x62, /* '6F': FCP Template */
                0x64, /* '6F': FMD Template */
                0x6F, /* FCI Template */
                0x80, /* '62': Data byte count */
                0x82, /* '62': File descriptor. */
                0x83, /* '62': File ID */
                0x84, /* '62': DF Name */
                0x88, /* '62': Short File ID */
                0x8A, /* '62': Life cycle status */
            };
            static uint32_t const tags_count = sizeof(tags) / sizeof(tags[0U]);
            swicc_dato_bertlv_tag_st bertlv_tags[tags_count];
            for (uint8_t tag_idx = 0U; tag_idx < tags_count; ++tag_idx)
            {
                if (swicc_dato_bertlv_tag_create(&bertlv_tags[tag_idx],
                                                 tags[tag_idx]) !=
                    SWICC_RET_SUCCESS)
                {
                    res->sw1 = SWICC_APDU_SW1_CHER_UNK;
                    res->sw2 = 0U;
                    res->data.len = 0U;
                    return SWICC_RET_SUCCESS;
                }
            }

            /* Create data for BER-TLV DOs. */
            uint32_t const data_size_be = htobe32(file_selected->data_size);
            uint16_t const data_id_be = htobe16(file_selected->hdr_file.id);
            uint8_t const data_sid[] = {file_selected->hdr_file.sid};
            uint8_t lcs_be;
            uint8_t descr_be[SWICC_FS_FILE_DESCR_LEN_MAX];
            uint8_t descr_len = 0U;
            if (swicc_fs_file_lcs(file_selected, &lcs_be) !=
                    SWICC_RET_SUCCESS ||
                swicc_fs_file_descr(swicc_state->fs.va.cur_tree, file_selected,
                                    descr_be, &descr_len) != SWICC_RET_SUCCESS)
            {
                res->sw1 = SWICC_APDU_SW1_CHER_UNK;
                res->sw2 = 0U;
                res->data.len = 0U;
                return SWICC_RET_SUCCESS;
            }

            uint8_t *bertlv_buf;
            uint32_t bertlv_len;
            swicc_ret_et ret_bertlv = SWICC_RET_ERROR;
            swicc_dato_bertlv_enc_st enc;
            for (bool dry_run = true;; dry_run = false)
            {
                if (dry_run)
                {
                    bertlv_buf = NULL;
                    bertlv_len = 0;
                }
                else
                {
                    if (enc.len > sizeof(res->data.b))
                    {
                        break;
                    }
                    bertlv_len = enc.len;
                    bertlv_buf = res->data.b;
                }

                swicc_dato_bertlv_enc_init(&enc, bertlv_buf, bertlv_len);
                swicc_dato_bertlv_enc_st enc_nstd;

                /* Nest everything in an FCI if it was requested. */
                if (data_req == DATA_REQ_FCI)
                {
                    if (swicc_dato_bertlv_enc_nstd_start(&enc, &enc_nstd) !=
                        SWICC_RET_SUCCESS)
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
                    swicc_dato_bertlv_enc_st enc_fcp;
                    if (swicc_dato_bertlv_enc_nstd_start(&enc_nstd, &enc_fcp) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_data(
                            &enc_fcp, (uint8_t *)&data_size_be,
                            sizeof(data_size_be)) != SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_hdr(&enc_fcp, &bertlv_tags[3U]) !=
                            SWICC_RET_SUCCESS ||
                        (file_selected->hdr_file.sid != 0
                             ? (swicc_dato_bertlv_enc_data(&enc_fcp, data_sid,
                                                           sizeof(data_sid)) !=
                                    SWICC_RET_SUCCESS ||
                                swicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                          &bertlv_tags[7U]) !=
                                    SWICC_RET_SUCCESS)
                             : false) ||
                        swicc_dato_bertlv_enc_data(&enc_fcp, &lcs_be,
                                                   sizeof(lcs_be)) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_hdr(&enc_fcp, &bertlv_tags[8U]) !=
                            SWICC_RET_SUCCESS ||
                        (file_selected->hdr_item.type ==
                                 SWICC_FS_ITEM_TYPE_FILE_MF
                             ? (swicc_dato_bertlv_enc_data(
                                    &enc_fcp, file_selected->hdr_spec.mf.name,
                                    SWICC_FS_NAME_LEN) != SWICC_RET_SUCCESS ||
                                swicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                          &bertlv_tags[6U]) !=
                                    SWICC_RET_SUCCESS)
                         : file_selected->hdr_item.type ==
                                 SWICC_FS_ITEM_TYPE_FILE_DF
                             ? (swicc_dato_bertlv_enc_data(
                                    &enc_fcp, file_selected->hdr_spec.df.name,
                                    SWICC_FS_NAME_LEN) != SWICC_RET_SUCCESS ||
                                swicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                          &bertlv_tags[6U]) !=
                                    SWICC_RET_SUCCESS)
                         : file_selected->hdr_item.type ==
                                 SWICC_FS_ITEM_TYPE_FILE_ADF
                             ? (swicc_dato_bertlv_enc_data(
                                    &enc_fcp,
                                    file_selected->hdr_spec.adf.aid.pix,
                                    sizeof(
                                        file_selected->hdr_spec.adf.aid.pix)) !=
                                    SWICC_RET_SUCCESS ||
                                swicc_dato_bertlv_enc_data(
                                    &enc_fcp,
                                    file_selected->hdr_spec.adf.aid.rid,
                                    sizeof(
                                        file_selected->hdr_spec.adf.aid.rid)) !=
                                    SWICC_RET_SUCCESS ||
                                swicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                          &bertlv_tags[6U]) !=
                                    SWICC_RET_SUCCESS)
                             : false) ||
                        (file_selected->hdr_file.id != 0
                             ? (swicc_dato_bertlv_enc_data(
                                    &enc_fcp, (uint8_t *)&data_id_be,
                                    sizeof(data_id_be)) != SWICC_RET_SUCCESS ||
                                swicc_dato_bertlv_enc_hdr(&enc_fcp,
                                                          &bertlv_tags[5U]) !=
                                    SWICC_RET_SUCCESS)
                             : false) ||
                        swicc_dato_bertlv_enc_data(&enc_fcp, descr_be,
                                                   descr_len) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_hdr(&enc_fcp, &bertlv_tags[4U]) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_nstd_end(&enc_nstd, &enc_fcp) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_hdr(
                            &enc_nstd, &bertlv_tags[0U]) != SWICC_RET_SUCCESS)
                    {
                        break;
                    }
                }
                /* Create an FMD if it was requested. */
                if (data_req == DATA_REQ_FCI || data_req == DATA_REQ_FMD)
                {
                    swicc_dato_bertlv_enc_st enc_fmd;
                    if (swicc_dato_bertlv_enc_nstd_start(&enc_nstd, &enc_fmd) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_nstd_end(&enc_nstd, &enc_fmd) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_hdr(
                            &enc_nstd, &bertlv_tags[1U]) != SWICC_RET_SUCCESS)
                    {
                        break;
                    }
                }
                if (data_req == DATA_REQ_FCI)
                {
                    if (swicc_dato_bertlv_enc_nstd_end(&enc, &enc_nstd) !=
                            SWICC_RET_SUCCESS ||
                        swicc_dato_bertlv_enc_hdr(&enc, &bertlv_tags[2U]) !=
                            SWICC_RET_SUCCESS)
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
                    ret_bertlv = SWICC_RET_SUCCESS;
                    break;
                }
            }

            /**
             * Check if BER-TLV creation succeeded and if enqueueing of the
             * BER-TLV DO succeeded.
             */
            if (ret_bertlv == SWICC_RET_SUCCESS &&
                swicc_apdu_rc_enq(&swicc_state->apdu_rc, res->data.b,
                                  bertlv_len) == SWICC_RET_SUCCESS)
            {
                res->sw1 = SWICC_APDU_SW1_NORM_BYTES_AVAILABLE;
                /**
                 * @todo What happens when extended APDUs are supported and
                 * length of response does not fit in SW2? For now work
                 * around is to static assert that only short APDUs are
                 * used.
                 */
                static_assert(SWICC_DATA_MAX == SWICC_DATA_MAX_SHRT,
                              "Response buffer length might not fit in SW2 "
                              "if SW1 is 0x61");
                /* Safe cast due to check inside the BER-TLV loop. */
                res->sw2 = (uint8_t)bertlv_len;
                res->data.len = 0U;
                return SWICC_RET_SUCCESS;
            }
            else
            {
                res->sw1 = SWICC_APDU_SW1_CHER_UNK;
                res->sw2 = 0U;
                res->data.len = 0U;
                return SWICC_RET_SUCCESS;
            }
        }
    }
}

/**
 * @brief Handle the READ BINARY command in the interindustry class.
 * @note As described in ISO/IEC 7816-4:2020 p.74 sec.11.3.3.
 */
static swicc_apduh_ft apduh_bin_read;
static swicc_ret_et apduh_bin_read(swicc_st *const swicc_state,
                                   swicc_apdu_cmd_st const *const cmd,
                                   swicc_apdu_res_st *const res,
                                   uint32_t const procedure_count)
{
    /**
     * Odd instruction (B1) not supported. It would have the data field encoded
     * as a BER-TLV DO.
     */
    if (cmd->hdr->ins != 0xB0)
    {
        res->sw1 = SWICC_APDU_SW1_CHER_INS;
        res->sw2 = 0U;
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    /**
     * This command does not take any data so sent a procedure to get all data
     * but set expected data length to be 0.
     */
    if (procedure_count == 0U)
    {
        res->sw1 = SWICC_APDU_SW1_PROC_ACK_ALL;
        res->sw2 = 0U;
        res->data.len = 0U; /* 0 bytes expected. */
        return SWICC_RET_SUCCESS;
    }
    else
    {
        /**
         * We sent an ACK ALL procedure and expected 0 bytes but got more than 0
         * bytes from the interface.
         */
        if (cmd->data->len != 0)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_LEN;
            res->sw2 = 0x02; /* The value of Lc is not the one expected. */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
    }

    uint8_t const len_expected = *cmd->p3;
    uint16_t offset;
    swicc_fs_file_st file;

    /**
     * Indicates if P1 contains a SID thus leading to a lookup and change in
     * current EF in VA on successful read.
     */
    bool const sid_use = cmd->hdr->p1 & 0b10000000;
    swicc_fs_sid_kt sid;

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
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        sid = cmd->hdr->p1 & 0b00011111;
        offset = cmd->hdr->p2;

        swicc_ret_et const ret_lookup =
            swicc_disk_lutsid_lookup(swicc_state->fs.va.cur_tree, sid, &file);
        if (ret_lookup == SWICC_RET_FS_NOT_FOUND)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x82; /* "File or application not found." */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
        else if (ret_lookup != SWICC_RET_SUCCESS)
        {
            /* Not sure what went wrong. */
            res->sw1 = SWICC_APDU_SW1_CHER_UNK;
            res->sw2 = 0U;
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
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

        file = swicc_state->fs.va.cur_ef;
        if (file.hdr_item.type == SWICC_FS_ITEM_TYPE_INVALID)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_CMD;
            res->sw2 = 0x86; /* "Command not allowed (curEF not set)" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
    }

    if (file.hdr_item.type == SWICC_FS_ITEM_TYPE_FILE_EF_TRANSPARENT)
    {
        if (offset >= file.data_size)
        {
            /**
             * Requested an offset which is outside the bounds of the
             * file.
             */
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2;
            res->sw2 = 0U;
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
        else if (offset + len_expected > file.data_size)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_LE;
            /* Safe cast since offset is less than data size. */
            res->sw2 = (uint8_t)(file.data_size - offset);
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        /* Read data into response. */
        memcpy(res->data.b, &file.data[offset], len_expected);
        res->data.len = len_expected;
        res->sw1 = SWICC_APDU_SW1_NORM_NONE;
        res->sw2 = 0U;

        if (sid_use)
        {
            /**
             * Select the file by SID now that the command is known to
             * succeed.
             */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
            /* SID is always initialized when sid_use is true so GCC is
             * detecting a false positive here. */
            if (swicc_va_select_file_sid(&swicc_state->fs, sid) ==
                SWICC_RET_SUCCESS)
#pragma GCC diagnostic pop
            {
                return SWICC_RET_SUCCESS;
            }
            /**
             * Selection should not fail since the lookup worked just fine.
             * This will fall-through to the unknown error.
             */
        }
        else
        {
            /* Nothing extra to be done on simple read by offset. */
            return SWICC_RET_SUCCESS;
        }
    }
    else
    {
        res->sw1 = SWICC_APDU_SW1_CHER_CMD;
        res->sw2 = 0x81; /* "Command incompatible with file structure" */
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    res->sw1 = SWICC_APDU_SW1_CHER_UNK;
    res->sw2 = 0U;
    res->data.len = 0U;
    return SWICC_RET_SUCCESS;
}

/**
 * @brief Handle the READ RECORD command in the interindustry class.
 * @note As described in ISO/IEC 7816-4:2020 p.82 sec.11.4.3.
 */
static swicc_apduh_ft apduh_rcrd_read;
static swicc_ret_et apduh_rcrd_read(swicc_st *const swicc_state,
                                    swicc_apdu_cmd_st const *const cmd,
                                    swicc_apdu_res_st *const res,
                                    uint32_t const procedure_count)
{
    /**
     * Odd instruction (B3) not supported. It would have the data field encoded
     * as a BER-TLV DO.
     */
    if (cmd->hdr->ins != 0xB2)
    {
        res->sw1 = SWICC_APDU_SW1_CHER_INS;
        res->sw2 = 0U;
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    /**
     * This command does not take any data so send a procedure to get all data
     * but set expected data length to be 0.
     */
    if (procedure_count == 0U)
    {
        res->sw1 = SWICC_APDU_SW1_PROC_ACK_ALL;
        res->sw2 = 0U;
        res->data.len = 0U; /* 0 bytes expected. */
        return SWICC_RET_SUCCESS;
    }
    else
    {
        /**
         * We sent an ACK ALL procedure and expected 0 bytes but got more than 0
         * bytes from the interface.
         */
        if (cmd->data->len != 0)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_LEN;
            res->sw2 = 0x02; /* The value of Lc is not the one expected. */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }
    }

    /* Where to read records from. */
    enum trgt_e
    {
        TRGT_EF_CUR,
        TRGT_EF_SID,
        TRGT_MANY,
    } trgt;

    /* Which occurrence to read when reading using an ID. */
    __attribute__((unused)) swicc_fs_occ_et occ = SWICC_FS_OCC_FIRST;

    /* What record(s) to read when reading using a number. */
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
    uint8_t const p2_target = (cmd->hdr->p2 & 0b11111000) >> 3U;

    /* Parse P1 and P2 */
    {
        switch (p2_target)
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
                occ = SWICC_FS_OCC_FIRST;
                break;
            case 0b01:
                occ = SWICC_FS_OCC_LAST;
                break;
            case 0b10:
                occ = SWICC_FS_OCC_NEXT;
                break;
            case 0b11:
                occ = SWICC_FS_OCC_PREV;
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
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x81; /* "Function not supported" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        /**
         * RFU values should never be received.
         * P1 = 0x00 is used for "special purposes" and P1 = 0xFF is RFU per
         * ISO/IEC 7816-4:2020 p.82 sec.11.4.2.
         * TODO: We ignore P1 = 0x00 for now. It indicates current record.
         * When the method of selecting record is by record number, ensure P1
         * (i.e. record number) is at least 1.
         */
        if ((trgt == TRGT_MANY && meth == METH_RCRD_ID) ||
            (trgt == TRGT_MANY && meth == METH_RCRD_NUM) || what == WHAT_RFU ||
            cmd->hdr->p1 == 0x00 || cmd->hdr->p1 == 0xFF ||
            (meth == METH_RCRD_NUM && cmd->hdr->p1 < 1))
        {
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        if (meth == METH_RCRD_NUM)
        {
            /* Safe cast because P1 is >0. */
            swicc_fs_rcrd_idx_kt const rcrd_idx = (uint8_t)(cmd->hdr->p1 - 1U);

            swicc_fs_file_st ef_cur;
            swicc_ret_et ret_ef = SWICC_RET_ERROR;
            switch (trgt)
            {
            case TRGT_EF_CUR: {
                ef_cur = swicc_state->fs.va.cur_ef;
                ret_ef = SWICC_RET_SUCCESS;
                break;
            }
            case TRGT_EF_SID: {
                swicc_fs_sid_kt const sid = p2_target;
                ret_ef = swicc_disk_lutsid_lookup(swicc_state->fs.va.cur_tree,
                                                  sid, &ef_cur);
                break;
            }
            default:
                /* Already rejected 'many' operation before. */
                __builtin_unreachable();
            }

            if (ret_ef == SWICC_RET_FS_NOT_FOUND)
            {
                res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
                res->sw2 = 0x82; /* "File or application not found" */
                res->data.len = 0U;
                return SWICC_RET_SUCCESS;
            }
            else if (ret_ef == SWICC_RET_SUCCESS)
            {
                /* Got the target EF, can read the record now. */
                uint8_t *rcrd_buf;
                uint8_t rcrd_len;
                swicc_ret_et const ret_rcrd =
                    swicc_disk_file_rcrd(swicc_state->fs.va.cur_tree, &ef_cur,
                                         rcrd_idx, &rcrd_buf, &rcrd_len);
                if (ret_rcrd == SWICC_RET_FS_NOT_FOUND)
                {
                    res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
                    res->sw2 = 0x83; /* "Record not found" */
                    res->data.len = 0U;
                    return SWICC_RET_SUCCESS;
                }
                else if (ret_rcrd == SWICC_RET_SUCCESS)
                {
                    /* Check if the expected response length is as expected. */
                    if (*cmd->p3 != rcrd_len)
                    {
                        /**
                         * Asking the interface to retry the command but this
                         * time with correct expected response length.
                         */
                        res->sw1 = SWICC_APDU_SW1_CHER_LE;
                        res->sw2 = rcrd_len;
                        res->data.len = 0U;
                        return SWICC_RET_SUCCESS;
                    }

                    /**
                     * Have to select the file on success (only if EF was
                     * selected by SID).
                     * @warning If this fails, something weird is going on.
                     */
                    swicc_ret_et const ret_file_select =
                        trgt == TRGT_EF_SID
                            ? swicc_va_select_file_sid(&swicc_state->fs,
                                                       ef_cur.hdr_file.sid)
                            : SWICC_RET_SUCCESS;
                    if (ret_file_select == SWICC_RET_SUCCESS)
                    {
                        /**
                         * In any case, have to select the record.
                         * @warning If this fails, something weird is going on.
                         */
                        swicc_ret_et const ret_rcrd_select =
                            swicc_va_select_record_idx(&swicc_state->fs,
                                                       rcrd_idx);
                        if (ret_rcrd_select == SWICC_RET_SUCCESS)
                        {
                            res->sw1 = SWICC_APDU_SW1_NORM_NONE;
                            res->sw2 = 0U;
                            res->data.len = rcrd_len;
                            memcpy(res->data.b, rcrd_buf, rcrd_len);
                            return SWICC_RET_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    res->sw1 = SWICC_APDU_SW1_CHER_UNK;
    res->sw2 = 0U;
    res->data.len = 0U;
    return SWICC_RET_SUCCESS;
}

/**
 * @brief Handle the UPDATE RECORD command in the interindustry class.
 * @note As described in ISO/IEC 7816-4:2020 p.82 sec.11.4.5.
 */
static swicc_apduh_ft apduh_rcrd_update;
static swicc_ret_et apduh_rcrd_update(swicc_st *const swicc_state,
                                      swicc_apdu_cmd_st const *const cmd,
                                      swicc_apdu_res_st *const res,
                                      uint32_t const procedure_count)
{
    /**
     * Odd instruction (DD) not supported. The data would contain an offset DO
     * and discretionary DO for encapsulating the updating data.
     */
    if (cmd->hdr->ins != 0xDC)
    {
        res->sw1 = SWICC_APDU_SW1_CHER_INS;
        res->sw2 = 0U;
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    /* Which records to update. */
    enum trgt_e
    {
        TRGT_EF_CUR,
        TRGT_EF_SID,
        TRGT_MANY,
    } trgt;

    /* Which occurrence to update when updating using an ID. */
    __attribute__((unused)) swicc_fs_occ_et occ = SWICC_FS_OCC_FIRST;

    /* What record(s) to update when updating using a number. */
    enum what_e
    {
        WHAT_P1,
        WHAT_P1_TO_LAST,
        WHAT_LAST_TO_P1,
        WHAT_RFU,
    } what = WHAT_RFU;

    /* Method of selecting a record to update. */
    enum meth_e
    {
        METH_RCRD_ID,
        METH_RCRD_NUM,
    } meth;

    /**
     * Value of first 5 bits. This can be interpreted differently based on the
     * other bits and P1.
     */
    uint8_t const p2_target = (cmd->hdr->p2 & 0b11111000) >> 3U;

    /* Parse P1 and P2 */
    {
        switch (p2_target)
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
                occ = SWICC_FS_OCC_FIRST;
                break;
            case 0b01:
                occ = SWICC_FS_OCC_LAST;
                break;
            case 0b10:
                occ = SWICC_FS_OCC_NEXT;
                break;
            case 0b11:
                occ = SWICC_FS_OCC_PREV;
                break;
            }
        }
    }

    /* Perform the requested operation. */
    {
        /**
         * Look at `apduh_rcrd_read`.
         */
        if (cmd->hdr->p2 == 0b11111000 || meth == METH_RCRD_ID ||
            trgt == TRGT_MANY)
        {
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x81; /* "Function not supported" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        /**
         * Look at `apduh_rcrd_read`.
         */
        if ((trgt == TRGT_MANY && meth == METH_RCRD_ID) ||
            (trgt == TRGT_MANY && meth == METH_RCRD_NUM) || what == WHAT_RFU ||
            cmd->hdr->p1 == 0x00 || cmd->hdr->p1 == 0xFF ||
            (meth == METH_RCRD_NUM && cmd->hdr->p1 < 1))
        {
            res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
            res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
            res->data.len = 0U;
            return SWICC_RET_SUCCESS;
        }

        if (meth == METH_RCRD_NUM)
        {
            /* Safe cast because P1 is >0. */
            swicc_fs_rcrd_idx_kt const rcrd_idx = (uint8_t)(cmd->hdr->p1 - 1U);

            swicc_fs_file_st ef_cur;
            swicc_ret_et ret_ef = SWICC_RET_ERROR;
            switch (trgt)
            {
            case TRGT_EF_CUR: {
                ef_cur = swicc_state->fs.va.cur_ef;
                ret_ef = SWICC_RET_SUCCESS;
                break;
            }
            case TRGT_EF_SID: {
                swicc_fs_sid_kt const sid = p2_target;
                ret_ef = swicc_disk_lutsid_lookup(swicc_state->fs.va.cur_tree,
                                                  sid, &ef_cur);
                break;
            }
            default:
                /* Already rejected 'many' operation before. */
                __builtin_unreachable();
            }

            if (ret_ef == SWICC_RET_FS_NOT_FOUND)
            {
                res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
                res->sw2 = 0x82; /* "File or application not found" */
                res->data.len = 0U;
                return SWICC_RET_SUCCESS;
            }
            else if (ret_ef == SWICC_RET_SUCCESS)
            {
                /* Got the target EF, can read the record now. */
                uint8_t *rcrd_buf;
                uint8_t rcrd_len;
                swicc_ret_et const ret_rcrd =
                    swicc_disk_file_rcrd(swicc_state->fs.va.cur_tree, &ef_cur,
                                         rcrd_idx, &rcrd_buf, &rcrd_len);
                if (ret_rcrd == SWICC_RET_FS_NOT_FOUND)
                {
                    res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
                    res->sw2 = 0x83; /* "Record not found" */
                    res->data.len = 0U;
                    return SWICC_RET_SUCCESS;
                }
                else if (ret_rcrd == SWICC_RET_SUCCESS)
                {
                    /**
                     * This command (INS=DC) expects the updated data to be sent
                     * over. Transmit a procedure byte to get all of it.
                     */
                    if (procedure_count == 0U)
                    {
                        res->sw1 = SWICC_APDU_SW1_PROC_ACK_ALL;
                        res->sw2 = 0U;
                        res->data.len = rcrd_len; /* Expecting as many bytes as
                                                     are in the record. */
                        return SWICC_RET_SUCCESS;
                    }
                    else
                    {
                        /**
                         * We sent an ACK ALL procedure and expected as many
                         * bytes as are in record but got a different amount
                         * from terminal.
                         */
                        if (cmd->data->len != rcrd_len)
                        {
                            res->sw1 = SWICC_APDU_SW1_CHER_LEN;
                            res->sw2 = 0x02; /* The value of Lc is not the one
                                                expected. */
                            res->data.len = rcrd_len;
                            return SWICC_RET_SUCCESS;
                        }
                    }

                    /* Check if the expected response length is as expected. */
                    if (*cmd->p3 != rcrd_len)
                    {
                        /**
                         * Asking the interface to retry the command but this
                         * time with correct expected response length.
                         */
                        res->sw1 = SWICC_APDU_SW1_CHER_LE;
                        res->sw2 = rcrd_len;
                        res->data.len = 0U;
                        return SWICC_RET_SUCCESS;
                    }

                    /**
                     * Have to select the file on success (only if EF was
                     * selected by SID).
                     * @warning If this fails, something weird is going on.
                     */
                    swicc_ret_et const ret_file_select =
                        trgt == TRGT_EF_SID
                            ? swicc_va_select_file_sid(&swicc_state->fs,
                                                       ef_cur.hdr_file.sid)
                            : SWICC_RET_SUCCESS;
                    if (ret_file_select == SWICC_RET_SUCCESS)
                    {
                        /**
                         * In any case, have to select the record.
                         * @warning If this fails, something weird is going on.
                         */
                        swicc_ret_et const ret_rcrd_select =
                            swicc_va_select_record_idx(&swicc_state->fs,
                                                       rcrd_idx);
                        if (ret_rcrd_select == SWICC_RET_SUCCESS)
                        {
                            /* Update the record. */
                            memcpy(rcrd_buf, cmd->data->b, rcrd_len);

                            res->sw1 = SWICC_APDU_SW1_NORM_NONE;
                            res->sw2 = 0U;
                            res->data.len = 0U;
                            return SWICC_RET_SUCCESS;
                        }
                    }
                }
            }
        }
    }

    res->sw1 = SWICC_APDU_SW1_CHER_UNK;
    res->sw2 = 0U;
    res->data.len = 0U;
    return SWICC_RET_SUCCESS;
}

/**
 * @brief Handle the GET RESPONSE command in the interindustry class.
 * @note As described in ISO/IEC 7816-4:2020 p.82 sec.11.4.3.
 */
static swicc_apduh_ft apduh_res_get;
static swicc_ret_et apduh_res_get(swicc_st *const swicc_state,
                                  swicc_apdu_cmd_st const *const cmd,
                                  swicc_apdu_res_st *const res,
                                  uint32_t const procedure_count)
{
    if (procedure_count == 0U)
    {
        res->sw1 = SWICC_APDU_SW1_PROC_ACK_ALL;
        res->sw2 = 0U;
        res->data.len = 0U; /* 0 bytes expected. */
        return SWICC_RET_SUCCESS;
    }
    else if (cmd->data->len != 0U)
    {
        /**
         * @note Lc field is not present in the command which implies data is
         * not present so if it sends data then the command is malformed since
         * Lc should have been present.
         */
        res->sw1 = SWICC_APDU_SW1_CHER_LEN;
        res->sw2 =
            0x01; /* "Command APDU format not compliant with this standard" */
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    /* P1 and P2 need to be 0, other values are RFU. */
    if (cmd->hdr->p1 != 0U || cmd->hdr->p2 != 0U)
    {
        res->sw1 = SWICC_APDU_SW1_CHER_P1P2_INFO;
        res->sw2 = 0x86; /* "Incorrect parameters P1-P2" */
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    /* Did not request any data. */
    if (*cmd->p3 == 0U)
    {
        res->sw1 = SWICC_APDU_SW1_NORM_NONE;
        res->sw2 = 0U;
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }

    uint32_t rc_len = *cmd->p3;
    swicc_ret_et const ret_rc =
        swicc_apdu_rc_deq(&swicc_state->apdu_rc, res->data.b, &rc_len);
    if (ret_rc == SWICC_RET_SUCCESS)
    {
        uint32_t const rc_len_rem =
            swicc_apdu_rc_len_rem(&swicc_state->apdu_rc);
        if (rc_len_rem > 0U)
        {
            res->sw1 = SWICC_APDU_SW1_NORM_BYTES_AVAILABLE;
            if (rc_len_rem > UINT8_MAX)
            {
                /**
                 * Can't indicate the real length remaining so just setting it
                 * to max.
                 */
                res->sw2 = 0xFF;
            }
            else
            {
                /* Safe cast since if checks if leq uint8 max. */
                res->sw2 = (uint8_t)rc_len_rem;
            }
            res->data.len = *cmd->p3;
            return SWICC_RET_SUCCESS;
        }
        else
        {
            res->sw1 = SWICC_APDU_SW1_NORM_NONE;
            res->sw2 = 0U;
            res->data.len = *cmd->p3;
            return SWICC_RET_SUCCESS;
        }
    }
    else
    {
        /* Not enough data in the RC buffer to give back to the interface. */
        res->sw1 = SWICC_APDU_SW1_WARN_NVM_CHGN;
        res->sw2 = 0x82; /* "End of file, record or DO reached before
                            reading Ne bytes, or unsuccessful search" */
        res->data.len = 0U;
        return SWICC_RET_SUCCESS;
    }
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_apduh_pro_register(swicc_st *const swicc_state,
                                      swicc_apduh_ft *const handler)
{
    swicc_state->internal.apduh_pro = handler;
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_apduh_demux(swicc_st *const swicc_state,
                               swicc_apdu_cmd_st const *const cmd,
                               swicc_apdu_res_st *const res,
                               uint32_t const procedure_count)
{
    swicc_ret_et ret = SWICC_RET_APDU_UNHANDLED;
    switch (cmd->hdr->cla.type)
    {
    case SWICC_APDU_CLA_TYPE_INVALID:
    case SWICC_APDU_CLA_TYPE_RFU:
        res->sw1 = SWICC_APDU_SW1_CHER_CLA; /* Marked as unsupported class. */
        res->sw2 = 0;
        res->data.len = 0;
        ret = SWICC_RET_SUCCESS;
        break;
    case SWICC_APDU_CLA_TYPE_INTERINDUSTRY:
        if (cmd->hdr->ins != 0xC0) /* GET RESPONSE instruction */
        {
            /* Make GET RESPONSE deterministically not work if resumed. */
            swicc_apdu_rc_reset(&swicc_state->apdu_rc);
        }

        /**
         * Let the interindustry implementations to be overridden by proprietary
         * ones.
         */
        if (swicc_state->internal.apduh_pro != NULL)
        {
            ret = swicc_state->internal.apduh_pro(swicc_state, cmd, res,
                                                  procedure_count);
            if (ret != SWICC_RET_APDU_UNHANDLED)
            {
                break;
            }
        }

        swicc_apduh_ft *apduh_func = apduh_unk;
        switch (cmd->hdr->ins)
        {
        case 0xA4:
            apduh_func = apduh_select;
            break;
        case 0xB0:
        case 0xB1:
            apduh_func = apduh_bin_read;
            break;
        case 0xB2:
        case 0xB3:
            apduh_func = apduh_rcrd_read;
            break;
        case 0xC0:
            apduh_func = apduh_res_get;
            break;
        case 0xDC:
        case 0xDD:
            apduh_func = apduh_rcrd_update;
            break;
        }
        ret = apduh_func(swicc_state, cmd, res, procedure_count);
        break;
    case SWICC_APDU_CLA_TYPE_PROPRIETARY:
        if (swicc_state->internal.apduh_pro == NULL)
        {
            ret = SWICC_RET_APDU_UNHANDLED;
            break;
        }
        ret = swicc_state->internal.apduh_pro(swicc_state, cmd, res,
                                              procedure_count);
        break;
    default:
        ret = SWICC_RET_APDU_UNHANDLED;
        break;
    }

    if (ret == SWICC_RET_APDU_UNHANDLED)
    {
        ret = SWICC_RET_SUCCESS;
        res->sw1 = SWICC_APDU_SW1_CHER_INS;
        res->sw2 = 0;
        res->data.len = 0;
    }
    else if (ret != SWICC_RET_SUCCESS)
    {
        ret = SWICC_RET_SUCCESS;
        res->sw1 = SWICC_APDU_SW1_CHER_UNK;
        res->sw2 = 0;
        res->data.len = 0;
    }
    return ret;
}

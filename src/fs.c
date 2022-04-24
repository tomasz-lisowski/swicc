#include "uicc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uicc_ret_et uicc_fs_reset(uicc_st *const uicc_state)
{
    memset(&uicc_state->internal.fs.va, 0U, sizeof(uicc_fs_va_st));
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_fs_disk_create(uicc_st *const uicc_state,
                                char const *const disk_json_path)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_disk_load(uicc_st *const uicc_state,
                              char const *const disk_path)
{
    uicc_ret_et ret = UICC_RET_FS_FAILURE;
    uicc_state->internal.fs.disk.buf = NULL;
    FILE *f = fopen(disk_path, "r");
    if (!(f == NULL))
    {
        if (fseek(f, 0, SEEK_END) == 0)
        {
            int64_t f_len_tmp = ftell(f);
            if (f_len_tmp >= 0)
            {
                if (fseek(f, 0, SEEK_SET) == 0)
                {
                    uicc_fs_magic_kt magic;
                    if (fread(&magic, UICC_FS_MAGIC_LEN, 1U, f) ==
                        UICC_FS_MAGIC_LEN)
                    {
                        if (memcmp(&magic, &(uicc_fs_magic_kt){UICC_FS_MAGIC},
                                   UICC_FS_MAGIC_LEN) == 0)
                        {
                            uint32_t const buf_len =
                                (uint32_t)f_len_tmp -
                                UICC_FS_MAGIC_LEN; /* Safe cast because it's
                                                    * checked if negative.
                                                    */
                            uicc_state->internal.fs.disk.buf = malloc(buf_len);
                            if (!(uicc_state->internal.fs.disk.buf == NULL))
                            {
                                uicc_state->internal.fs.disk.len = buf_len;
                                if (fread(uicc_state->internal.fs.disk.buf,
                                          buf_len, 1U, f) == buf_len)
                                {
                                    ret = UICC_RET_SUCCESS;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (fclose(f) != 0)
        {
            /* TODO: Should the func set a failure error code here? */
        }
    }
    if (ret != UICC_RET_SUCCESS && uicc_state->internal.fs.disk.buf != NULL)
    {
        free(uicc_state->internal.fs.disk.buf);
        uicc_state->internal.fs.disk.buf = NULL;
        uicc_state->internal.fs.disk.len = 0;
    }
    return ret;
}

uicc_ret_et uicc_fs_disk_save(uicc_st *const uicc_state,
                              char const *const disk_path)
{
    uicc_ret_et ret = UICC_RET_FS_FAILURE;
    FILE *f = fopen(disk_path, "w");
    if (fwrite(&(uicc_fs_magic_kt){UICC_FS_MAGIC}, UICC_FS_MAGIC_LEN, 1U, f) ==
        UICC_FS_MAGIC_LEN)
    {
        if (fwrite(&uicc_state->internal.fs.disk.buf,
                   uicc_state->internal.fs.disk.len, 1U,
                   f) == uicc_state->internal.fs.disk.len)
        {
            ret = UICC_RET_SUCCESS;
        }
        if (fclose(f) != 0)
        {
            /* TODO: Should the func set a failure error code here? */
        }
    }
    return ret;
}

uicc_ret_et uicc_fs_disk_unload(uicc_st *const uicc_state)
{
    free(uicc_state->internal.fs.disk.buf);
    memset(&uicc_state->internal.fs, 0U, sizeof(uicc_fs_st));
    return UICC_RET_SUCCESS;
}

uicc_ret_et uicc_fs_select_file_dfname(uicc_st *const uicc_state,
                                       char const *const df_name,
                                       uint32_t const df_name_len)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_file_fid(uicc_st *const uicc_state,
                                    uicc_fs_fid_kt const fid)
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
                                     uicc_fs_record_id_kt id)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_record_idx(uicc_st *const uicc_state,
                                      uicc_fs_record_idx_kt idx)
{
    return UICC_RET_UNKNOWN;
}

uicc_ret_et uicc_fs_select_data_offset(uicc_st *const uicc_state,
                                       uint32_t offset)
{
    return UICC_RET_UNKNOWN;
}

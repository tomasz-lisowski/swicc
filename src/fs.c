#include <string.h>
#include <uicc/uicc.h>

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

#include <string.h>
#include <swicc/swicc.h>

void swicc_apdu_rc_reset(swicc_apdu_rc_st *const rc)
{
    if (rc != NULL)
    {
        memset(rc, 0U, sizeof(*rc));
    }
}

swicc_ret_et swicc_apdu_rc_enq(swicc_apdu_rc_st *const rc,
                               uint8_t const *const buf, uint32_t const buf_len)
{
    if (rc == NULL || buf == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    static_assert(sizeof(rc->b) <= UINT32_MAX,
                  "Size of the RC buffer does not fit in a uint32.");

    /* Check if the new data will fit. */
    if (rc->len + buf_len > sizeof(rc->b))
    {
        return SWICC_RET_BUFFER_TOO_SHORT;
    }

    memcpy(&rc->b[rc->len], buf, buf_len);
    /* Safe cast since it was checked to not overflow the type. */
    rc->len = (uint32_t)(rc->len + buf_len);
    return SWICC_RET_SUCCESS;
}

swicc_ret_et swicc_apdu_rc_deq(swicc_apdu_rc_st *const rc, uint8_t *const buf,
                               uint32_t *const buf_len)
{
    if (rc == NULL || buf == NULL || buf_len == NULL)
    {
        return SWICC_RET_PARAM_BAD;
    }

    /**
     * Safe cast since the offset is guaranteed to never move further than the
     * length.
     */
    uint32_t const rc_len_rem = (uint32_t)(rc->len - rc->offset);
    if (*buf_len <= rc_len_rem)
    {
        memcpy(buf, &rc->b[rc->offset], *buf_len);

        /**
         * Safe cast since the additon will not be greater than the RC length
         * which fits in uint32.
         */
        rc->offset = (uint32_t)(rc->offset + *buf_len);
        return SWICC_RET_SUCCESS;
    }
    else
    {
        *buf_len = rc_len_rem;
        return SWICC_RET_BUFFER_TOO_SHORT;
    }
}

uint32_t swicc_apdu_rc_len_rem(swicc_apdu_rc_st const *const rc)
{
    if (rc == NULL)
    {
        return 0U;
    }
    /* Safe cast since RC length is never less than the offset. */
    return (uint32_t)(rc->len - rc->offset);
}

#include <tau/tau.h>

#include <swicc/swicc.h>

TEST(apdu_rc, swicc_apdu_rc_reset)
{
    swicc_apdu_rc_st rc = {
        .len = 10U,
        .offset = 2U,
    };
    swicc_apdu_rc_reset(&rc);
    CHECK_EQ(rc.len, 0U);
    CHECK_EQ(rc.offset, 0U);
}

TEST(apdu_rc, swicc_apdu_rc_enq__param_check)
{
    swicc_apdu_rc_st *const rc = (swicc_apdu_rc_st *)1U;
    uint8_t const *const buf = (uint8_t *)1U;
    uint32_t const buf_len = 0U;
    CHECK_EQ(swicc_apdu_rc_enq(NULL, buf, buf_len), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_apdu_rc_enq(rc, NULL, buf_len), SWICC_RET_PARAM_BAD);
}

TEST(apdu_rc, swicc_apdu_rc_enq__data)
{
    uint8_t buf_enq[SWICC_DATA_MAX + 1U];
    for (uint32_t buf_enq_idx = 0U; buf_enq_idx < sizeof(buf_enq);
         ++buf_enq_idx)
    {
        /* Safe cast due to the modulo operation. */
        buf_enq[buf_enq_idx] = (uint8_t)(buf_enq_idx % UINT8_MAX);
    }

    swicc_apdu_rc_st rc;
    swicc_apdu_rc_reset(&rc);
    CHECK_EQ(sizeof(rc.b), sizeof(buf_enq) - 1U);
    CHECK_EQ(swicc_apdu_rc_enq(&rc, buf_enq, sizeof(buf_enq)),
             SWICC_RET_BUFFER_TOO_SHORT);
    CHECK_EQ(swicc_apdu_rc_enq(&rc, buf_enq, sizeof(buf_enq) - 1U),
             SWICC_RET_SUCCESS);
    CHECK_BUF_EQ(rc.b, buf_enq, sizeof(buf_enq) - 1U);
    CHECK_EQ(rc.len, sizeof(buf_enq) - 1U);
    CHECK_EQ(rc.offset, 0U);
}

TEST(apdu_rc, swicc_apdu_rc_deq__param_check)
{
    swicc_apdu_rc_st *const rc = (swicc_apdu_rc_st *)1U;
    uint8_t *const buf = (uint8_t *)1U;
    uint32_t *const buf_len = (uint32_t *)1U;
    CHECK_EQ(swicc_apdu_rc_deq(NULL, buf, buf_len), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_apdu_rc_deq(rc, NULL, buf_len), SWICC_RET_PARAM_BAD);
    CHECK_EQ(swicc_apdu_rc_deq(rc, buf, NULL), SWICC_RET_PARAM_BAD);
}

TEST(apdu_rc, swicc_apdu_rc_deq__data)
{
    uint8_t buf_enq[SWICC_DATA_MAX];
    for (uint32_t buf_enq_idx = 0U; buf_enq_idx < sizeof(buf_enq);
         ++buf_enq_idx)
    {
        /* Safe cast due to the modulo operation. */
        buf_enq[buf_enq_idx] = (uint8_t)(buf_enq_idx % UINT8_MAX);
    }

    uint8_t buf_deq[SWICC_DATA_MAX] = {0U};
    uint32_t buf_deq_len_tot = 0U;
    uint32_t buf_deq_len_exp = sizeof(buf_deq);
    uint32_t buf_deq_len = buf_deq_len_exp;

    swicc_apdu_rc_st rc;
    swicc_apdu_rc_reset(&rc);
    CHECK_EQ(sizeof(rc.b), sizeof(buf_enq));
    CHECK_EQ(swicc_apdu_rc_enq(&rc, buf_enq, buf_deq_len_exp),
             SWICC_RET_SUCCESS);

    /* Dequeuing whole buffer. */
    CHECK_EQ(swicc_apdu_rc_deq(&rc, buf_deq, &buf_deq_len), SWICC_RET_SUCCESS);
    CHECK_EQ(buf_deq_len, buf_deq_len_exp);
    {
        int32_t const tmp = (int32_t)buf_deq_len;
        CHECK_BUF_EQ(buf_deq, buf_enq, (size_t)tmp);
    }
    CHECK_EQ(rc.offset, buf_deq_len_exp);

    swicc_apdu_rc_reset(&rc);
    memset(buf_deq, 0U, sizeof(buf_deq));
    static_assert(
        SWICC_DATA_MAX >= 55U,
        "swICC data len max is less than 55 which makes this test invalid.");
    buf_deq_len_exp = 55U;
    buf_deq_len = buf_deq_len_exp;

    /* Dequeuing part of the buffer multiple times. */
    CHECK_EQ(swicc_apdu_rc_enq(&rc, buf_enq, sizeof(buf_enq)),
             SWICC_RET_SUCCESS);
    CHECK_EQ(swicc_apdu_rc_deq(&rc, buf_deq, &buf_deq_len), SWICC_RET_SUCCESS);
    CHECK_EQ(buf_deq_len, buf_deq_len_exp);
    {
        int32_t const tmp = (int32_t)buf_deq_len;
        CHECK_BUF_EQ(buf_deq, buf_enq, (size_t)tmp);
    }
    CHECK_EQ(rc.offset, buf_deq_len_exp);
    /* Safe cast since this will not  */
    buf_deq_len_tot += buf_deq_len;

    static_assert(
        SWICC_DATA_MAX >= 55U + 124,
        "swICC data len max is less than 55 + 124 which makes this test invalid.");
    buf_deq_len_exp = 124U;
    buf_deq_len = buf_deq_len_exp;

    CHECK_EQ(swicc_apdu_rc_deq(&rc, buf_deq, &buf_deq_len), SWICC_RET_SUCCESS);
    CHECK_EQ(buf_deq_len, buf_deq_len_exp);
    {
        int32_t const tmp = (int32_t)buf_deq_len_exp;
        CHECK_BUF_EQ(buf_deq, &buf_enq[buf_deq_len_tot], (size_t)tmp);
    }
    CHECK_EQ(rc.offset, buf_deq_len_tot + buf_deq_len_exp);
    buf_deq_len_tot += buf_deq_len;

    /**
     * Safe cast since the enq buffer is never smaller than the total len
     * dequeued.
     */
    buf_deq_len_exp = (uint32_t)(sizeof(buf_enq) - buf_deq_len_tot);

    /* 1 too much from the max length that can be dequeued. */
    /**
     * Safe cast since this is less than the enqueue buffer length which fits in
     * the uint32 type.
     */
    buf_deq_len = (uint32_t)(buf_deq_len_exp + 1U);

    CHECK_EQ(swicc_apdu_rc_deq(&rc, buf_deq, &buf_deq_len),
             SWICC_RET_BUFFER_TOO_SHORT);

    buf_deq_len = buf_deq_len_exp;
    CHECK_EQ(swicc_apdu_rc_deq(&rc, buf_deq, &buf_deq_len), SWICC_RET_SUCCESS);

    CHECK_EQ(buf_deq_len, buf_deq_len_exp);
    {
        int32_t const tmp = (int32_t)buf_deq_len_exp;
        CHECK_BUF_EQ(buf_deq, &buf_enq[buf_deq_len_tot], (size_t)tmp);
    }
    CHECK_EQ(rc.offset, buf_deq_len_tot + buf_deq_len_exp);
    CHECK_EQ(rc.offset, rc.len);
    buf_deq_len_tot += buf_deq_len;
}

TEST(apdu_rc, swicc_apdu_rc_len_rem)
{
    swicc_apdu_rc_st rc;
    swicc_apdu_rc_reset(&rc);
    uint8_t buf[SWICC_DATA_MAX];
    for (uint32_t buf_idx = 0U; buf_idx < sizeof(buf); ++buf_idx)
    {
        /* Safe cast due to the modulo operation. */
        buf[buf_idx] = (uint8_t)(buf_idx % UINT8_MAX);
    }
    CHECK_EQ(swicc_apdu_rc_enq(&rc, buf, sizeof(buf)), SWICC_RET_SUCCESS);
    CHECK_EQ(swicc_apdu_rc_len_rem(&rc), sizeof(buf));

    static_assert(
        sizeof(buf) > 20,
        "Size of buffer is not at least 20 making this test invalid.");
    uint32_t len_deq = sizeof(buf) - 20U;
    CHECK_EQ(swicc_apdu_rc_deq(&rc, buf, &len_deq), SWICC_RET_SUCCESS);
    CHECK_EQ(swicc_apdu_rc_len_rem(&rc), 20U);
}

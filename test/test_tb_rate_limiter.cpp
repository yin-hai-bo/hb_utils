#include <gtest/gtest.h>
#include <memory>
#include "../src/tb_rate_limiter.h"

typedef std::unique_ptr<TBRateLimiter> limiter_ptr;

using Action = TBRateLimiter::Action;

TEST(TBRateLimiter, Generic) {
    const TBRateLimiter::Params params {
        1000,   // CIR
        5000,   // CBS
        1000,   // EIR
        6000,   // EBS
    };
    TBRateLimiter trl(params, 0);
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // C bucket is full as beginning.
    ASSERT_EQ(0, trl.GetEBucketTokens());                   // And E bucket is empty.

    uint64_t now = 0;
    ASSERT_EQ(Action::DENY, trl.Execute(5001, now));        // Deny (C bucket has 5000 only, and E bucket is empty)

    now += 5001;
    ASSERT_EQ(Action::DENY, trl.Execute(5002, now));
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // The count of token remains unchanged, even request has been rejected.
    ASSERT_EQ(5001, trl.GetEBucketTokens());                // Now, there are 5001 tokens in the E bucket.

    now += 1;
    ASSERT_EQ(Action::ALLOW, trl.Execute(5002, now));      // E bucket: 5002
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // C bucket remains unchanged.
    ASSERT_EQ(0, trl.GetEBucketTokens());                   // E bucket is empty now.

    ASSERT_EQ(Action::ALLOW, trl.Execute(5000, now));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    now += 1;
    ASSERT_EQ(Action::DENY, trl.Execute(2, now));           // There are one token in the C bucket (1ms elapsed)
    now += 1;
    ASSERT_EQ(Action::ALLOW, trl.Execute(2, now));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    now += 5700;
    ASSERT_EQ(Action::DENY, trl.Execute(100000, now));
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // 5700 ms elapsed, C bucket is full.
    ASSERT_EQ(700, trl.GetEBucketTokens());                 // And 700 tokens into E bucket.

    now += 1000000;
    ASSERT_EQ(Action::ALLOW, trl.Execute(5700, now));
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // C bucket remains unchanged (should allocate from E bucket)
    ASSERT_EQ(300, trl.GetEBucketTokens());                 // There are 300 tokens remain in the E bucket.
}

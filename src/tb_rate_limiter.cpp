#include "tb_rate_limiter.h"

namespace yhb {

TBRateLimiter::TBRateLimiter(const Params & params, uint64_t now)
    : last_time(now)
    , bucket_committed(params.committed_burst_size, params.committed_info_rate / 1000, true)
    , bucket_excess(params.excess_burst_size, params.excess_info_rate / 1000, false)
{}

TBRateLimiter::Action TBRateLimiter::Execute(size_t size, uint64_t now) {
    uint64_t elapsed;
    if (now > this->last_time) {
        elapsed = now - this->last_time;
        this->last_time = now;
    } else {
        elapsed = 0;
    }

    unsigned const remain_time = bucket_committed.Put(static_cast<unsigned>(elapsed));
    if (remain_time != 0) {
        bucket_excess.Put(remain_time);
    }

    // First, try to take tokens from the C bucket.
    if (bucket_committed.Acquire(size)) {
        return Action::ALLOW;
    }

    // Not enough tokens in the C bucket, try to take from E bucket.
    if (bucket_excess.Acquire(size)) {
        return Action::ALLOW;
    }

    // Both two buckets are full
    return Action::DENY;
}

/**
 * @brief Construct a bucket.
 *
 * @param size[in]      Maximum size of the bucket, in bytes.
 * @param info_rate[in] Speed of traffic passing.
 *                      How many tokens into bucket per milliseconds.
 * @param init_full[in] Full the bucket when initialization.
 */
TBRateLimiter::Bucket::Bucket(uint64_t size, uint64_t info_rate, bool init_full)
    : size(size)
    , info_rate(info_rate)
    , tokens(init_full ? size : 0) {}

/**
 * @brief Try to throw tokens into the bucket.
 *
 * @param elapsed_milliseconds[in]  Elapsed milliseconds though last call Put().
 *                                  The count of tokens which need throw into bucket equals
 *                                  'elapsed_milliseconds' times 'info_rate'.
 * @return A time value in milliseconds. ('elapsed_milliseconds' subtract consumed time).
 */
unsigned TBRateLimiter::Bucket::Put(unsigned elapsed_milliseconds) {
    // When bucket full, no time consumed, return the elapsed time immediately.
    uint64_t const remain_space = this->size - this->tokens;
    if (remain_space == 0) {
        return elapsed_milliseconds;
    }

    // How long does it take to fill the bucket?
    // If elapsed time large than the need time, fill the bucket, then return
    // the remain time with consumed.
    uint64_t const need_time = remain_space / this->info_rate;
    if (elapsed_milliseconds >= need_time) {
        this->tokens = this->size;
        return static_cast<unsigned>(elapsed_milliseconds - need_time);
    }
    // Too small elapsed time, throw some tokens into bucket, return zero.
    this->tokens += elapsed_milliseconds * info_rate;
    return 0;
}

/**
 * @brief   Try to acquire token.
 *
 * @param count[in] How many tokens do I want acquire.
 * @return          When there are enough tokens in bucket, consume 'count' tokens
 *                  and return true. otherwise return false, and the tokens in bucket
 *                  remain unchanged.
 */
bool TBRateLimiter::Bucket::Acquire(size_t count) {
    if (count <= this->tokens) {
        this->tokens -= count;
        return true;
    }
    return false;
}

} // End of namespace 'yhb'
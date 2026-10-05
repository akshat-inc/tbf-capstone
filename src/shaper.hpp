#pragma once
#include <deque>
#include <algorithm>
#include "token_bucket.hpp"

class Shaper {
public:
    Shaper(double rate_bps, double burst, size_t qmax) : tb_(rate_bps, burst), qmax_(qmax) {}
    bool enqueue(uint64_t bytes, uint64_t now_ns, uint64_t &release_ns) {
        while (!q_.empty() && q_.front() <= now_ns) q_.pop_front();
        if (q_.size() >= qmax_) { dropped_++; return false; }
        uint64_t t = std::max(now_ns, last_release_);
        t += tb_.wait_ns(bytes, t);
        tb_.consume(bytes, t);
        last_release_ = t;
        q_.push_back(t);
        release_ns = t;
        sent_++;
        return true;
    }
    uint64_t sent() const { return sent_; }
    uint64_t dropped() const { return dropped_; }
private:
    TokenBucket tb_;
    size_t qmax_;
    std::deque<uint64_t> q_;
    uint64_t last_release_ = 0;
    uint64_t sent_ = 0;
    uint64_t dropped_ = 0;
};

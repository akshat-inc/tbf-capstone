#pragma once
#include "token_bucket.hpp"

class Policer {
public:
    Policer(double rate_bps, double burst) : tb_(rate_bps, burst) {}
    bool admit(uint64_t bytes, uint64_t now_ns) {
        bool ok = tb_.consume(bytes, now_ns);
        if (ok) passed_++; else dropped_++;
        return ok;
    }
    uint64_t passed() const { return passed_; }
    uint64_t dropped() const { return dropped_; }
private:
    TokenBucket tb_;
    uint64_t passed_ = 0;
    uint64_t dropped_ = 0;
};

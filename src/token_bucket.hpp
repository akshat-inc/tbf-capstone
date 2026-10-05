#pragma once
#include <cstdint>
#include <algorithm>

class alignas(64) TokenBucket {
public:
    TokenBucket(double rate_bps, double burst) : rate_(rate_bps), burst_(burst), tokens_(burst), last_ns_(0) {}
    void refill(uint64_t now_ns) {
        if (now_ns <= last_ns_) return;
        tokens_ = std::min(burst_, tokens_ + (now_ns - last_ns_) / 1e9 * rate_);
        last_ns_ = now_ns;
    }
    bool consume(uint64_t bytes, uint64_t now_ns) {
        refill(now_ns);
        if (tokens_ < (double)bytes) return false;
        tokens_ -= bytes;
        return true;
    }
    uint64_t wait_ns(uint64_t bytes, uint64_t now_ns) {
        refill(now_ns);
        if (tokens_ >= (double)bytes) return 0;
        return (uint64_t)((bytes - tokens_) / rate_ * 1e9) + 1;
    }
private:
    double rate_;
    double burst_;
    double tokens_;
    uint64_t last_ns_;
};

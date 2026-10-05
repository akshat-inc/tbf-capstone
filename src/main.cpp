#include <cstdio>
#include <algorithm>
#include "token_bucket.hpp"

int main() {
    const uint64_t pkt = 1000;
    const uint64_t gap_ns = 1000000;
    TokenBucket pol(500000, 5000);
    int passed = 0, dropped = 0;
    for (int i = 0; i < 1000; i++) {
        if (pol.consume(pkt, i * gap_ns)) passed++; else dropped++;
    }
    printf("policer: passed=%d dropped=%d\n", passed, dropped);
    TokenBucket shp(500000, 5000);
    uint64_t release = 0, total_delay = 0;
    for (int i = 0; i < 1000; i++) {
        uint64_t now = i * gap_ns;
        uint64_t t = std::max(now, release);
        t += shp.wait_ns(pkt, t);
        shp.consume(pkt, t);
        release = t;
        total_delay += t - now;
    }
    printf("shaper: avg delay=%.1f ms\n", total_delay / 1000.0 / 1e6);
    return 0;
}

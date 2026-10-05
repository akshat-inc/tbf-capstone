#pragma once
#include <cstdint>
#include <random>
#include <string>
#include <vector>

struct Pkt { uint64_t t_ns; uint32_t size; };

inline std::vector<Pkt> generate(const std::string &mode, int n, uint64_t gap_ns, uint32_t size, uint64_t seed) {
    std::vector<Pkt> v;
    std::mt19937_64 rng(seed);
    std::exponential_distribution<double> expd(1.0 / (double)gap_ns);
    uint64_t t = 0;
    for (int i = 0; i < n; i++) {
        v.push_back({t, size});
        if (mode == "constant") t += gap_ns;
        else if (mode == "burst") t += (i % 10 == 9) ? gap_ns * 10 - 90000 : 10000;
        else t += (uint64_t)expd(rng);
    }
    return v;
}

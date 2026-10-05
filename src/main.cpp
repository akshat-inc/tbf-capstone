#include <cstdio>
#include <cstdlib>
#include <string>
#include "policer.hpp"
#include "shaper.hpp"
#include "traffic.hpp"

int main(int argc, char **argv) {
    std::string mode = "constant";
    std::string csv = "out.csv";
    double rate = 500000;
    double burst = 5000;
    int count = 1000;
    uint32_t pkt = 1000;
    uint64_t gap_us = 1000;
    size_t qmax = 64;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string k = argv[i];
        std::string v = argv[i + 1];
        if (k == "--mode") mode = v;
        else if (k == "--rate") rate = atof(v.c_str());
        else if (k == "--burst") burst = atof(v.c_str());
        else if (k == "--count") count = atoi(v.c_str());
        else if (k == "--pkt") pkt = (uint32_t)atoi(v.c_str());
        else if (k == "--gap-us") gap_us = (uint64_t)atoll(v.c_str());
        else if (k == "--queue") qmax = (size_t)atoi(v.c_str());
        else if (k == "--csv") csv = v;
        else { fprintf(stderr, "unknown option %s\n", k.c_str()); return 1; }
    }
    if (mode != "constant" && mode != "burst" && mode != "random") { fprintf(stderr, "mode must be constant, burst or random\n"); return 1; }
    if ((double)pkt > burst) { fprintf(stderr, "pkt size must be <= burst\n"); return 1; }
    std::vector<Pkt> pkts = generate(mode, count, gap_us * 1000, pkt, 42);
    Policer pol(rate, burst);
    Shaper shp(rate, burst, qmax);
    FILE *f = fopen(csv.c_str(), "w");
    if (!f) { perror("csv"); return 1; }
    fprintf(f, "idx,arrival_ms,bytes,policer,shaper,delay_ms\n");
    double total_delay_ms = 0;
    for (size_t i = 0; i < pkts.size(); i++) {
        bool ok = pol.admit(pkts[i].size, pkts[i].t_ns);
        uint64_t rel = 0;
        bool sent = shp.enqueue(pkts[i].size, pkts[i].t_ns, rel);
        double d = sent ? (double)(rel - pkts[i].t_ns) / 1e6 : -1.0;
        if (sent) total_delay_ms += d;
        fprintf(f, "%zu,%.3f,%u,%s,%s,%.3f\n", i, pkts[i].t_ns / 1e6, pkts[i].size, ok ? "pass" : "drop", sent ? "sent" : "drop", d);
    }
    fclose(f);
    double secs = pkts.back().t_ns / 1e9;
    printf("mode=%s rate=%.0f B/s burst=%.0f B packets=%d\n", mode.c_str(), rate, burst, count);
    printf("offered load : %.0f B/s\n", count * (double)pkt / secs);
    printf("policer      : passed=%lu dropped=%lu\n", (unsigned long)pol.passed(), (unsigned long)pol.dropped());
    printf("shaper       : sent=%lu dropped=%lu avg_delay=%.1f ms\n", (unsigned long)shp.sent(), (unsigned long)shp.dropped(), shp.sent() ? total_delay_ms / shp.sent() : 0.0);
    printf("csv written  : %s\n", csv.c_str());
    return 0;
}

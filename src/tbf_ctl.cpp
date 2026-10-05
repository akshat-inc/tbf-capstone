#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include "../include/tbf_ioctl.h"

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: tbf_ctl config <rate> <burst> | send <count> <size> <gap_us> | stats | reset\n"); return 1; }
    int fd = open("/dev/tbf", O_RDWR);
    if (fd < 0) { perror("open /dev/tbf (is the module loaded?)"); return 1; }
    std::string c = argv[1];
    if (c == "config" && argc == 4) {
        tbf_config cfg = { strtoull(argv[2], 0, 10), strtoull(argv[3], 0, 10) };
        if (ioctl(fd, TBF_SET_CONFIG, &cfg) < 0) { perror("ioctl config"); return 1; }
        printf("configured rate=%s B/s burst=%s B\n", argv[2], argv[3]);
    } else if (c == "send" && argc == 5) {
        int n = atoi(argv[2]);
        uint32_t size = (uint32_t)atoi(argv[3]);
        long gap = atol(argv[4]);
        int ok = 0, drop = 0;
        timespec ts = { gap / 1000000, (gap % 1000000) * 1000 };
        for (int i = 0; i < n; i++) {
            ssize_t r = write(fd, &size, sizeof(size));
            if (r == (ssize_t)sizeof(size)) ok++;
            else if (errno == EAGAIN) drop++;
            else { perror("write"); return 1; }
            nanosleep(&ts, 0);
        }
        printf("sent=%d passed=%d dropped=%d\n", n, ok, drop);
    } else if (c == "stats") {
        tbf_stats s;
        if (ioctl(fd, TBF_GET_STATS, &s) < 0) { perror("ioctl stats"); return 1; }
        printf("passed=%llu dropped=%llu passed_bytes=%llu dropped_bytes=%llu\n", (unsigned long long)s.passed, (unsigned long long)s.dropped, (unsigned long long)s.passed_bytes, (unsigned long long)s.dropped_bytes);
    } else if (c == "reset") {
        if (ioctl(fd, TBF_RESET) < 0) { perror("ioctl reset"); return 1; }
        printf("reset\n");
    } else { fprintf(stderr, "bad command\n"); return 1; }
    close(fd);
    return 0;
}

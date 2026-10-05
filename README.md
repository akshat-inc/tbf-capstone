# Token-Bucket Network Bandwidth Shaper & Traffic Policer

C++ user-space shaper/policer plus a Linux kernel character-device policer (/dev/tbf).

## Build and run (user space)
    make
    ./tbf --mode constant|burst|random --rate 500000 --burst 5000 --count 1000 --csv out.csv

## Kernel driver (needs a real Linux kernel, tested in an Ubuntu VM)
    sudo apt install build-essential linux-headers-$(uname -r)
    cd kernel && make
    sudo insmod tbf_dev.ko
    cd .. && make ctl
    ./tbf_ctl config 500000 5000
    ./tbf_ctl send 1000 1000 1000
    ./tbf_ctl stats
    cat /proc/tbf_stats
    sudo rmmod tbf_dev

## Demo
    ./scripts/run_demo.sh

## Design
- Token bucket: rate (bytes/s), burst (max bytes), lazy refill from a monotonic clock
- Policer: drops packets that exceed the rate
- Shaper: delays packets to the configured rate, bounded queue
- Driver: misc char device, write() verdict, ioctl config/stats/reset, spinlock, /proc/tbf_stats

See docs/ARCHITECTURE.md for details.

# Architecture

## Components
- src/token_bucket.hpp: token bucket, cache-line aligned (alignas(64)), lazy refill from a nanosecond timestamp
- src/policer.hpp: admits a packet if enough tokens exist, otherwise drops it
- src/shaper.hpp: computes a release time for each packet so the output never exceeds the rate, bounded queue drops overflow
- src/traffic.hpp: constant, burst and random (Poisson) packet generators
- src/main.cpp: command line simulator, writes a CSV per run
- kernel/tbf_dev.c: Linux misc character device /dev/tbf
- src/tbf_ctl.cpp: user-space tool using open, write and ioctl on /dev/tbf

## Kernel driver design
- write(fd, &size, 4): returns 4 if the packet passes, -EAGAIN if dropped
- ioctl TBF_SET_CONFIG, TBF_GET_STATS, TBF_RESET
- spinlock (irqsave) protects the bucket because several processes may write concurrently
- tokens are stored as integer byte-nanoseconds because the kernel avoids floating point
- ktime_get_ns() gives a monotonic clock
- /proc/tbf_stats exposes live counters

## Hardware and software concepts
- user space to kernel boundary: syscalls, copy_from_user, copy_to_user
- cache-line alignment of the hot data structure
- monotonic clock source, no wall-clock jumps
- integer fixed-point arithmetic in kernel context

#pragma once
#include <linux/ioctl.h>
#include <linux/types.h>
struct tbf_config { __u64 rate_bps; __u64 burst; };
struct tbf_stats { __u64 passed; __u64 dropped; __u64 passed_bytes; __u64 dropped_bytes; };
#define TBF_MAGIC 't'
#define TBF_SET_CONFIG _IOW(TBF_MAGIC, 1, struct tbf_config)
#define TBF_GET_STATS _IOR(TBF_MAGIC, 2, struct tbf_stats)
#define TBF_RESET _IO(TBF_MAGIC, 3)

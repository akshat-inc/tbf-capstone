#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "../include/tbf_ioctl.h"

#define NS 1000000000ULL
static DEFINE_SPINLOCK(tbf_lock);
static u64 rate = 500000;
static u64 burst = 5000;
static u64 tokens = 5000 * NS;
static u64 last_ns;
static struct tbf_stats st;

static void refill(u64 now)
{
    u64 cap = burst * NS;
    u64 el = now - last_ns;
    last_ns = now;
    if (el > div64_u64(cap, rate)) tokens = cap;
    else tokens = min(cap, tokens + el * rate);
}

static void snap(struct tbf_stats *s, u64 *r, u64 *b)
{
    unsigned long flags;
    spin_lock_irqsave(&tbf_lock, flags);
    *s = st;
    *r = rate;
    *b = burst;
    spin_unlock_irqrestore(&tbf_lock, flags);
}

static ssize_t tbf_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    u32 size;
    u64 cost;
    unsigned long flags;
    int ok;
    if (len != sizeof(size)) return -EINVAL;
    if (copy_from_user(&size, buf, sizeof(size))) return -EFAULT;
    cost = (u64)size * NS;
    spin_lock_irqsave(&tbf_lock, flags);
    refill(ktime_get_ns());
    ok = tokens >= cost;
    if (ok) tokens -= cost;
    if (ok) st.passed++;
    if (ok) st.passed_bytes += size;
    if (!ok) st.dropped++;
    if (!ok) st.dropped_bytes += size;
    spin_unlock_irqrestore(&tbf_lock, flags);
    return ok ? (ssize_t)len : -EAGAIN;
}

static ssize_t tbf_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    char tmp[200];
    struct tbf_stats s;
    u64 r, b;
    int n;
    snap(&s, &r, &b);
    n = scnprintf(tmp, sizeof(tmp), "rate=%llu burst=%llu passed=%llu dropped=%llu passed_bytes=%llu dropped_bytes=%llu\n", r, b, s.passed, s.dropped, s.passed_bytes, s.dropped_bytes);
    return simple_read_from_buffer(buf, len, off, tmp, n);
}

static long tbf_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct tbf_config cfg;
    struct tbf_stats s;
    u64 r, b;
    unsigned long flags;
    switch (cmd) {
    case TBF_SET_CONFIG:
        if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg))) return -EFAULT;
        if (cfg.rate_bps == 0 || cfg.rate_bps > 1000000000ULL) return -EINVAL;
        if (cfg.burst == 0 || cfg.burst > 1000000000ULL) return -EINVAL;
        spin_lock_irqsave(&tbf_lock, flags);
        rate = cfg.rate_bps;
        burst = cfg.burst;
        tokens = burst * NS;
        last_ns = ktime_get_ns();
        spin_unlock_irqrestore(&tbf_lock, flags);
        return 0;
    case TBF_GET_STATS:
        snap(&s, &r, &b);
        if (copy_to_user((void __user *)arg, &s, sizeof(s))) return -EFAULT;
        return 0;
    case TBF_RESET:
        spin_lock_irqsave(&tbf_lock, flags);
        memset(&st, 0, sizeof(st));
        tokens = burst * NS;
        last_ns = ktime_get_ns();
        spin_unlock_irqrestore(&tbf_lock, flags);
        return 0;
    }
    return -ENOTTY;
}

static int tbf_proc_show(struct seq_file *m, void *v)
{
    struct tbf_stats s;
    u64 r, b;
    snap(&s, &r, &b);
    seq_printf(m, "rate_bytes_per_s: %llu\nburst_bytes: %llu\npassed: %llu\ndropped: %llu\npassed_bytes: %llu\ndropped_bytes: %llu\n", r, b, s.passed, s.dropped, s.passed_bytes, s.dropped_bytes);
    return 0;
}

static int tbf_proc_open(struct inode *i, struct file *f) { return single_open(f, tbf_proc_show, NULL); }

static const struct proc_ops tbf_proc_ops = { .proc_open = tbf_proc_open, .proc_read = seq_read, .proc_lseek = seq_lseek, .proc_release = single_release };
static const struct file_operations tbf_fops = { .owner = THIS_MODULE, .read = tbf_read, .write = tbf_write, .unlocked_ioctl = tbf_ioctl, .llseek = noop_llseek };
static struct miscdevice tbf_misc = { .minor = MISC_DYNAMIC_MINOR, .name = "tbf", .fops = &tbf_fops, .mode = 0666 };

static int __init tbf_init(void)
{
    int ret;
    last_ns = ktime_get_ns();
    ret = misc_register(&tbf_misc);
    if (ret) return ret;
    proc_create("tbf_stats", 0444, NULL, &tbf_proc_ops);
    pr_info("tbf: loaded, /dev/tbf and /proc/tbf_stats ready\n");
    return 0;
}

static void __exit tbf_exit(void)
{
    remove_proc_entry("tbf_stats", NULL);
    misc_deregister(&tbf_misc);
    pr_info("tbf: unloaded\n");
}

module_init(tbf_init);
module_exit(tbf_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Akshat");
MODULE_DESCRIPTION("Token bucket policer character device");

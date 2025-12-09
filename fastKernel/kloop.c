// kloop.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");

static int __init kloop_init(void)
{
    const long iterations = 100000000; // 100 million
    long x = 0;

    ktime_t t0 = ktime_get();
    for (long i = 0; i < iterations; i++) {
        x++;
    }
    ktime_t t1 = ktime_get();

    s64 ns = ktime_to_ns(ktime_sub(t1, t0));
    s64 ms = ns / 1000000; 
    s64 us = ns/ 1000; 
    printk(KERN_INFO "kernel-loop: %lld ns (%lld us, %lld ms)\n",
           ns, us, ms);

    return 0;
}

static void __exit kloop_exit(void)
{
    printk(KERN_INFO "kernel-loop module unloaded\n");
}

module_init(kloop_init);
module_exit(kloop_exit);


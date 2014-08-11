#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junwei Zhang");
MODULE_DESCRIPTION("A Simple to test spinlock performance");

static int set_affinity(int cpu)
{
	struct cpumask mask;

	cpumask_empty(&mask);
	cpumask_set_cpu(cpu, &mask);
	set_cpus_allowed_ptr(current, &mask);

	printk(KERN_DEBUG "Run on cpu:%d\n", smp_processor_id());
	return 0;
}

static int show_freq(int cpu)
{
	unsigned int freq;
	freq = cpufreq_quick_get(cpu);
	printk(KERN_DEBUG "cpu%d freq:%u\n", cpu, freq);

	return 0;
}

static int __init test_init(void)
{
	spinlock_t  lock;
	u64 count = 1024*1024*50;
	u64 i;
	//unsigned long start, end;
	unsigned long flags;
	struct timespec start, end;
	uint32_t jiff_s, jiff_e;
	unsigned long long tsc_s, tsc_e;
	int cpu = 0;

	spin_lock_init(&lock);
	set_affinity(cpu);
	show_freq(cpu);
	local_irq_save(flags);

	getnstimeofday(&start);
	jiff_s = jiffies;
	tsc_s = native_read_tsc();
	for(i=0; i<count; i++) {
		spin_lock(&lock);
		spin_unlock(&lock);
	}
	tsc_e = native_read_tsc();
	jiff_e = jiffies;
	getnstimeofday(&end);

	local_irq_restore(flags);
	show_freq(cpu);

	printk(KERN_DEBUG "start: %ld,%ld\n", start.tv_sec, start.tv_nsec);
	printk(KERN_DEBUG "end: %ld,%ld\n", end.tv_sec, end.tv_nsec);
	printk(KERN_DEBUG "offset:%ld, %ld\n", end.tv_sec - start.tv_sec,
		(end.tv_sec - start.tv_sec)*1000000 + (end.tv_nsec - start.tv_nsec)/1000);

	printk(KERN_DEBUG "HZ:%u\n", HZ);
	printk(KERN_DEBUG "Jiffies %u,%u, %u\n", jiff_e, jiff_s, jiffies_to_msecs(jiff_e - jiff_s));

	printk(KERN_DEBUG "TSC %llu,%llu, delta %llu\n", tsc_s, tsc_e, (tsc_e - tsc_s) >> 20);
	return 0;
}

static void __exit test_exit(void)
{
	printk(KERN_DEBUG "spinlock test exit\n");
	return;
}

module_init(test_init);
module_exit(test_exit);

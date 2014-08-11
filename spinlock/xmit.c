#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junwei Zhang");
MODULE_DESCRIPTION("A Simple to test spinlock performance");

static int __init test_init(void)
{
	u64 i = 1024*1024;
	//unsigned long start, end;
	spinlock_t  lock;
	unsigned long flags;
	struct timespec start, end;
	uint32_t jiff_s, jiff_e;

	spin_lock_init(&lock);
	local_irq_save(flags);

	getnstimeofday(&start);
	jiff_s = jiffies;
	while(i-->0) {
		spin_lock(&lock);
		spin_unlock(&lock);
	}
	jiff_e = jiffies;
	getnstimeofday(&end);

	local_irq_restore(flags);

	printk(KERN_DEBUG "start: %ld,%ld\nend: %ld,%ld\noffset:%ld, %ld\n",
		start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec,
		end.tv_sec - start.tv_sec,
		(end.tv_sec - start.tv_sec)*1000000 + (end.tv_nsec - start.tv_nsec)/1000);
	printk(KERN_DEBUG "HZ:%u\n", HZ);
	printk(KERN_DEBUG "Jiffies %u,%u, %u\n", jiff_e, jiff_s, jiffies_to_msecs(jiff_e - jiff_s));
	return 0;
}

static void __exit test_exit(void)
{
	printk(KERN_DEBUG "spinlock test exit\n");
	return;
}

module_init(test_init);
module_exit(test_exit);

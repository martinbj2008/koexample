#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/kernel.h>
#include <net/dst.h>
#include <net/tcp.h>
#include <net/inet_common.h>
#include <linux/ipsec.h>
#include <asm/unaligned.h>


int hook__dev_mc_add(struct net_device *dev, const unsigned char *addr, bool global)
{
	if (dev)
		printk(KERN_INFO "%s: dev:%s,  mac:%02x:%02x:%02x:%02x:%02x:%02x, global:%d\n", __func__, dev->name,
			addr[0], addr[1], addr[2],addr[3], addr[4], addr[5],
			global);
	dump_stack();
	jprobe_return();
	return 0;
}

static struct jprobe hook_dev_mc_add = {
	.entry          = hook__dev_mc_add,
	.kp.symbol_name = "__dev_mc_add"
};

static int __init kprobe_init(void)
{
	int ret;

	ret = register_jprobe(&hook_dev_mc_add);
	if (ret == 0)
		printk(KERN_INFO "Register mc hook OK\n");

	return ret;
}

static void __exit kprobe_exit(void)
{
	unregister_jprobe(&hook_dev_mc_add);
	printk(KERN_INFO "Unregister mc hook OK\n");
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");

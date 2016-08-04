#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>

static __net_init int show_ns_init(struct net *net)
{
	struct net_device *dev;

	printk(KERN_DEBUG "%s: net:%p\n", __func__, net);

	list_for_each_entry(dev, &net->dev_base_head, dev_list) {
		printk(KERN_DEBUG "   dev:%s\n", dev->name);
	}

	return 0;
}

/* Registered in net/core/dev.c */
struct pernet_operations __net_initdata show_ns_ops = {
       .init = show_ns_init,
};

int __init tcp_hook_init(void)
{
	if (register_pernet_device(&show_ns_ops))
		return -1;
	unregister_pernet_device(&show_ns_ops);
	return 0;
}

void tcp_hook_exit(void)
{
	return;
}

module_init(tcp_hook_init);
module_exit(tcp_hook_exit);
MODULE_LICENSE("GPL");

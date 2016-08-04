#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>

struct net_device *dev;
static int drop=0;

/* Called with rcu_read_lock and bottom-halves disabled. */
static rx_handler_result_t netdev_frame_hook(struct sk_buff **pskb)
{
	struct sk_buff *skb = *pskb;
	struct iphdr *iph;
	struct tcphdr *tcph;

	if (skb_headlen(skb) < sizeof(struct iphdr) + sizeof(struct tcphdr))
		return RX_HANDLER_PASS;
	iph = (struct iphdr *)skb->data;
	tcph = (struct tcphdr*)(iph+1);

	if (drop == 0 && skb->len > 100 && skb->len < 1400 && tcph->dest == htons(1000)) {
		printk(KERN_DEBUG "%pI4:%u --> %pI4:%u, ip ID:%u, skb:%p, len:%d, data_len:%d\n",
				&iph->saddr, htons(tcph->source), &iph->daddr, htons(tcph->dest),
				ntohs(iph->id), skb, skb->len, skb->data_len);
		drop = 1;
		return RX_HANDLER_CONSUMED;
	}
	return RX_HANDLER_PASS;
}

int __init tcp_hook_init(void)
{
	int ret;
	dev = dev_get_by_name(&init_net, "ens3");

	if (dev == NULL)
		return -EINVAL;
	rtnl_lock();
	ret  = netdev_rx_handler_register(dev, netdev_frame_hook, dev);
	rtnl_unlock();
	printk(KERN_DEBUG "%s:  ret:%d\n", __func__, ret);

	return 0;
}

void tcp_hook_exit(void)
{
	rtnl_lock();
	netdev_rx_handler_unregister(dev);
	rtnl_unlock();
}

module_init(tcp_hook_init);
module_exit(tcp_hook_exit);
MODULE_DESCRIPTION("TCP packet filtere");
MODULE_LICENSE("GPL");

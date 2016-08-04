#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/net_namespace.h>
#include <net/inet_hashtables.h>
#include <net/tcp.h>

int packet_skb_recv(struct sk_buff *skb, struct net_device *dev,
                  struct packet_type *ptype, struct net_device *orig_dev)
{
	struct iphdr *iph;
	struct udphdr *udph;
	int min_len = sizeof(struct udphdr) + sizeof(struct iphdr);

	if (!(skb->dev && skb->dev == dev))
		goto fin;

	if (!pskb_may_pull(skb, min_len))
		goto fin;

	iph = (struct iphdr *)skb->data;

	if (iph->ihl < 5 || iph->version != 4)
		goto fin;
	if (iph->ihl != 5)
		goto fin;
	if (skb->len < iph->ihl*4 +sizeof(struct udphdr))
		goto fin;

	if (iph->protocol != IPPROTO_UDP)
		goto fin;

	udph = (struct udphdr *)(((uint8_t *)iph) + iph->ihl*4);

	printk(KERN_DEBUG "%s: %pI4(%u) --> %pI4(%u) dir:%s\n",
		__func__,
		&iph->saddr,
		ntohs(udph->source),
		&iph->daddr,
		ntohs(udph->dest),
		skb->pkt_type == PACKET_OUTGOING ? "out" : "in" );
fin:
	dev_kfree_skb(skb);
	return 0;
}

static struct packet_type packet_hook __read_mostly = {
	.type = cpu_to_be16(ETH_P_IP),
	.func = packet_skb_recv, 
};

int __init tcp_hook_init(void)
{
	dev_add_pack(&packet_hook);

	return 0;
}

void tcp_hook_exit(void)
{
	dev_remove_pack(&packet_hook);
}

module_init(tcp_hook_init);
module_exit(tcp_hook_exit);
MODULE_LICENSE("GPL");

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

struct net_device *dev;

/* Called with rcu_read_lock and bottom-halves disabled. */
static rx_handler_result_t netdev_frame_hook(struct sk_buff **pskb)
{
	int ret;
	struct sk_buff *skb = *pskb;
	struct iphdr *iph;
	struct tcphdr *tcph;
	struct sock *sk;
	int min_len = sizeof(struct tcphdr) + sizeof(struct iphdr);

	ret = RX_HANDLER_PASS;
	if (!(skb->dev && skb->dev == dev))
		goto fin;

	if (skb->protocol != cpu_to_be16(ETH_P_IP))
		goto fin;
	if (skb->len < min_len)
		goto fin;

	if (!pskb_may_pull(skb, min_len))
		goto fin;

	iph = (struct iphdr *)skb->data;

	if (iph->ihl < 5 || iph->version != 4)
		goto fin;
	if (iph->ihl != 5)
		goto fin;
	if (skb->len < iph->ihl*4 +sizeof(struct tcphdr))
		goto fin;
	tcph = (struct tcphdr *)(((uint8_t *)iph) + iph->ihl*4);

	local_bh_disable();
	sk = __inet_lookup_established(&init_net, &tcp_hashinfo,
			iph->saddr, tcph->source,
			iph->daddr, ntohs(tcph->dest), skb->skb_iif);
	local_bh_enable();

	if (!sk)
		goto fin;

	switch (sk->sk_state) {
		case TCP_LAST_ACK:
			if (tcph->ack && !tcph->syn && !tcph->fin) {
				ret = RX_HANDLER_CONSUMED;
			}
		case TCP_FIN_WAIT1:
		case TCP_FIN_WAIT2:
			printk(KERN_INFO "%s:  sk state:%d, %pI4(%u) --> %pI4(%u) skb:%p, len:%d, seq:%u, syn:%d, rst:%d, ack:%d, fin:%d\n",
					__func__, sk->sk_state,
					&inet_sk(sk)->inet_daddr,
					ntohs(inet_sk(sk)->inet_dport),
					&inet_sk(sk)->inet_saddr,
					ntohs(inet_sk(sk)->inet_sport),
					skb, skb->len, ntohl(tcph->seq),
					tcph->syn, tcph->rst,
					tcph->ack, tcph->fin);
			break;
		default:
			break;
	}

	// dirty !!
	skb_shinfo(skb)->ip6_frag_id = tcph->seq;
	sock_put(sk);
	if (ret == RX_HANDLER_CONSUMED)
		kfree(skb);

fin:
	return ret;
}

int __init tcp_hook_init(void)
{
	int ret;
	uint8_t dev_name[] = "ens3";

	dev = dev_get_by_name(&init_net, dev_name);

	if (dev == NULL) {
		printk(KERN_DEBUG "%s: Failed to find nic:%s\n", __func__, dev_name);
		return -EINVAL;
	}

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
	local_bh_disable();
	dev_put(dev);
	local_bh_enable();
}

module_init(tcp_hook_init);
module_exit(tcp_hook_exit);
MODULE_LICENSE("GPL");

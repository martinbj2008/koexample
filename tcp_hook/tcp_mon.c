#include <linux/kernel.h>
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

#if 0
int j_tcp_v4_do_rcv(struct sock *sk, struct sk_buff *skb)
{
	struct tcphdr *th = tcp_hdr(skb);
	if (sk->sk_state == TCP_LAST_ACK) {
		printk(KERN_INFO "%s:  sk state:%d, %pI4(%u) --> %pI4(%u) skb:%p, len:%d, seq:%u, syn:%d, rst:%d, syn:%d, ack:%d, fin:%d\n",
				__func__, sk->sk_state,
				&inet_sk(sk)->inet_saddr,
				ntohs(inet_sk(sk)->inet_sport),
				&inet_sk(sk)->inet_daddr,
				ntohs(inet_sk(sk)->inet_dport),
				skb, skb->len, ntohl(th->seq),
				th->syn, th->rst, th->syn,
				th->ack, th->fin);
	}

	jprobe_return();

	return 0;
}

int j_tcp_rcv_state_process(struct sock *sk, struct sk_buff *skb,
	const struct tcphdr *th, unsigned int len)
{
	if (sk->sk_state == TCP_LAST_ACK) {
		printk(KERN_INFO "%s:  sk state:%d, %pI4(%u) --> %pI4(%u) skb:%p, len:%d, seq:%u, syn:%d, rst:%d, syn:%d, ack:%d, fin:%d\n",
				__func__, sk->sk_state,
				&inet_sk(sk)->inet_saddr,
				ntohs(inet_sk(sk)->inet_sport),
				&inet_sk(sk)->inet_daddr,
				ntohs(inet_sk(sk)->inet_dport),
				skb, len, ntohl(th->seq),
				th->syn, th->rst, th->syn,
				th->ack, th->fin);
	}

	jprobe_return();
	return 0;
}

static struct jprobe tcp_hook_jp = {
	.entry          = j_tcp_v4_do_rcv,
	.kp.symbol_name = "tcp_v4_do_rcv"
};

#else

int j_kfree_skb(struct sk_buff *skb)
{
	if (skb && skb->head) {
		if (skb_shinfo(skb)->ip6_frag_id)
			printk(KERN_INFO "%s: skb:%p, %u\n",
					__func__, skb, skb_shinfo(skb)->ip6_frag_id);
	}
	jprobe_return();
	return 0;
}

static struct jprobe tcp_hook_jp = {
	.entry          = j_kfree_skb,
	.kp.symbol_name = "kfree_skb"
};
#endif

static int __init kprobe_init(void)
{
	int ret;

	ret = register_jprobe(&tcp_hook_jp);
	if (ret == 0)
		printk(KERN_INFO "Register tcp hook OK\n");

	return ret;
}

static void __exit kprobe_exit(void)
{
	unregister_jprobe(&tcp_hook_jp);
	printk(KERN_INFO "Unregister tcp hook OK\n");
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");

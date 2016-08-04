#include <linux/kernel.h>
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

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <net/netfilter/nf_conntrack_seqadj.h>
#include <net/netfilter/nf_conntrack_synproxy.h>
#include <net/netfilter/nf_log.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <net/netfilter/ipv6/nf_conntrack_ipv6.h>

static const char *const tcp_conntrack_names[] = {
	"NONE",
	"SYN_SENT",
	"SYN_RECV",
	"ESTABLISHED",
	"FIN_WAIT",
	"CLOSE_WAIT",
	"LAST_ACK",
	"TIME_WAIT",
	"CLOSE",
	"SYN_SENT2",
};

struct tcp_packet {
	u32 jiff;
	u32 sip;
	u32 dip;
	struct tcphdr tcph;
};

struct port_pkt {
	u8 fin_idx;
	u8 syn_idx;
	u8 ack_idx;
	u8 res;
	struct  tcp_packet fin_pkt[4];
	struct  tcp_packet syn_pkt[4];
	struct  tcp_packet ack_pkt[8];
};

struct port_pkt *port_pkt_db;

static struct ctl_table_header * tcp_monitor_sysctl_hdr;
uint16_t tcp_port;

void pkt_print(struct tcp_packet *pkt)
{
	printk(KERN_INFO "%s: jiff:%u, %pI4(%u)-->%pI4(%u) seq:%u, syn:%d fin:%d ack:%d\n",
			__func__,
			pkt->jiff,
			&(pkt->sip),
			ntohs(pkt->tcph.source),
			&(pkt->dip),
			ntohs(pkt->tcph.dest),
			ntohs(pkt->tcph.seq),
			pkt->tcph.syn,
			pkt->tcph.fin,
			pkt->tcph.ack);
}

int debug_proc_dointvec(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int i;
	int ret;
	struct port_pkt *tmp;

	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	tmp = &port_pkt_db[htons(tcp_port)];

	printk(KERN_INFO "%s: port: %u, syn:%u, fin:%u, ack:%u\n",
			__func__, tcp_port, tmp->syn_idx,
			tmp->ack_idx, tmp->ack_idx);

	for (i=0; i<4;  i++)
		pkt_print(&tmp->syn_pkt[i]);

	for (i=0; i<4; i++)
		pkt_print(&tmp->fin_pkt[i]);

	for (i=0; i<8; i++)
		pkt_print(&tmp->ack_pkt[i]);

	return ret;
}

static struct ctl_table tcp_monitor_table[] = {
	{
		.procname       = "tcp_port",
		.data           = &tcp_port,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = debug_proc_dointvec,
	},
	{}
};

/* Returns verdict for packet, or -1 for invalid. */
int (*kern_tcp_packet)(struct nf_conn *ct,
		const struct sk_buff *skb,
		unsigned int dataoff,
		enum ip_conntrack_info ctinfo,
		u_int8_t pf,
		unsigned int hooknum,
		unsigned int *timeouts);

int log_nf_event(struct nf_conn *ct, const struct tcphdr *th, const struct sk_buff *skb)
{
	enum tcp_conntrack tc_tcp_state;
	struct nf_conntrack_tuple *tuple;

	tc_tcp_state = ct->proto.tcp.state;

	tuple = &ct->tuplehash[0].tuple;
	printk(KERN_INFO "%s: %pI4:%u ==> %pI4:%u ct state: %s, th syn:%d, ack:%d, fin:%d, seq:%u, skb:%p, len:%d\n",
			__func__,
			&tuple->src.u3.ip,
			ntohs(tuple->src.u.tcp.port),
			&tuple->dst.u3.ip,
			ntohs(tuple->dst.u.tcp.port),
			tcp_conntrack_names[ct->proto.tcp.state],
			th->syn,
			th->ack,
			th->fin,
			ntohs(th->seq),
			skb,
			skb->len);
	return 0;
}

int pkt_store(const struct tcphdr *tcph, u32 sip, u32 dip, struct tcp_packet *pkt)
{
	pkt->jiff = jiffies;
	pkt->sip = sip;
	pkt->dip = dip;

	memcpy(&pkt->tcph, tcph, sizeof(struct tcphdr));

	return 0;
}

/* Returns verdict for packet, or -1 for invalid. */
static int debug_tcp_packet(struct nf_conn *ct,
		const struct sk_buff *skb,
		unsigned int dataoff,
		enum ip_conntrack_info ctinfo,
		u_int8_t pf,
		unsigned int hooknum,
		unsigned int *timeouts)
{
	const struct tcphdr *th;
	struct tcphdr _tcph;
	struct port_pkt *tmp ;
	struct nf_conntrack_tuple *tuple;

	th = skb_header_pointer(skb, dataoff, sizeof(_tcph), &_tcph);
	BUG_ON(th == NULL);
	BUG_ON(kern_tcp_packet == NULL);
#if 0	
	if (th->syn && th->ack ==0 && ct->proto.tcp.state != TCP_CONNTRACK_NONE)
		log_nf_event(ct, th, skb);
	else if (ct->proto.tcp.state == TCP_CONNTRACK_LAST_ACK)
		log_nf_event(ct, th, skb);
#endif
printk(KERN_INFO "%s: port:%u, %u\n", __func__, ntohs(th->source), ntohs(th->dest));
	tuple = &ct->tuplehash[0].tuple;
	tmp = &port_pkt_db[ntohs(tuple->src.u.tcp.port)];
	if (th->syn) {
		int idx = tmp->syn_idx & 0x03;
		pkt_store(th, tuple->src.u3.ip, tuple->dst.u3.ip, &tmp->syn_pkt[idx]);
		tmp->syn_idx++;
	}
	else if (th->fin) {
		int idx = tmp->fin_idx & 0x03;
		pkt_store(th, tuple->src.u3.ip, tuple->dst.u3.ip, &tmp->fin_pkt[idx]);
		tmp->fin_idx++;
	}
	else if (th->ack && tmp->fin_idx) {
		int idx = tmp->ack_idx & 0x07;
		pkt_store(th, tuple->src.u3.ip, tuple->dst.u3.ip, &tmp->ack_pkt[idx]);
		tmp->ack_idx++;
	}
	return kern_tcp_packet(ct, skb, dataoff, ctinfo, pf, hooknum, timeouts);
}

static int __init nf_tcp_packet_init(void)
{
	u32 size = 65536*sizeof(struct port_pkt);

	struct nf_conntrack_l4proto *kern_nf_conntrack_l4proto_tcp4 = &nf_conntrack_l4proto_tcp4;

	tcp_monitor_sysctl_hdr = register_net_sysctl(&init_net, "net/ipv4", tcp_monitor_table);
	port_pkt_db = vzalloc(size);
	if (port_pkt_db == NULL)
		return -ENOMEM;	

	if (kern_nf_conntrack_l4proto_tcp4->packet == NULL)
		return -EINVAL;
	kern_tcp_packet = kern_nf_conntrack_l4proto_tcp4->packet;
	kern_nf_conntrack_l4proto_tcp4->packet = debug_tcp_packet;
	printk(KERN_INFO "%s: Replace packet function(%pF ==> %pF).\n",
			__func__, kern_tcp_packet, debug_tcp_packet);

	printk(KERN_INFO "%s: NF TCP PACKET Init OK\n", __func__);
	return 0;
}

static void __exit nf_tcp_packet_exit(void)
{
	struct nf_conntrack_l4proto *kern_nf_conntrack_l4proto_tcp4 = &nf_conntrack_l4proto_tcp4;

	if (kern_nf_conntrack_l4proto_tcp4->packet == debug_tcp_packet) {
		kern_nf_conntrack_l4proto_tcp4->packet = kern_tcp_packet;
		printk(KERN_INFO "%s: Restore packet function to %pF\n", __func__, kern_tcp_packet);
	}

	vfree(port_pkt_db);
	unregister_sysctl_table(tcp_monitor_sysctl_hdr);
	printk(KERN_INFO "%s: NF TCP PACKET Exit.\n", __func__);
	return;
}

module_init(nf_tcp_packet_init)
module_exit(nf_tcp_packet_exit)
MODULE_LICENSE("GPL");

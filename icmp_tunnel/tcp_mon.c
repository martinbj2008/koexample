#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <net/tcp.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define HTTPS_PORT 443

static struct kprobe kp = {
	//.symbol_name	= "dev_queue_xmit",
	.symbol_name	= "dev_hard_start_xmit",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	struct sk_buff *skb;
//	long exit_code = (long)regs->di;
//	pr_info("handler_pre: name [%s], pid [%d], exit_code [%ld] \n", 
//		current->comm, current->pid, exit_code);

	struct ethhdr *eth;
	struct iphdr *iph;
	struct tcphdr *tcph;
	struct icmphdr *icmph;

	skb = (struct sk_buff *)regs_get_kernel_argument(regs, 0);
	if (skb == NULL)
		goto fin;

	// linear?
	if (skb->len - skb->data_len < 14 + 20 + 20) //eth+ ip + tcp
		goto fin;

	//printk(KERN_INFO "%s: skb:%p, len:%u\n", __func__, skb, skb->len);
	eth = (struct ethhdr *)(skb->data);
	iph = (struct iphdr*)(eth + 1);
	tcph = (struct tcphdr*)(iph + 1);  //maybe ip option

	if (eth->h_proto != cpu_to_be16(ETH_P_IP))
		goto fin;
	if (iph->protocol != IPPROTO_TCP)
		goto fin;
	if (tcph->dest != htons(HTTPS_PORT))
		goto fin;

	if (iph->ihl != 5) {
		printk(KERN_ERR "IP options!!!\n");
		goto fin;
	}

	printk(KERN_INFO "%s: len:%u, %pI4:%u  -> %pI4:%u \n", __func__, skb->len,
			&iph->saddr, ntohs(tcph->source), 
			&iph->daddr, ntohs(tcph->dest));

	iph->protocol = IPPROTO_ICMP;
	icmph = (struct icmphdr *)tcph;	
	skb_push(skb, sizeof(struct icmphdr));
	memmove(skb->data, eth, 14+20);
	icmph->type = 0;
	icmph->code = 0;
	icmph->checksum = 0;
	icmph->un.echo.id = 0;
	icmph->un.echo.sequence = 0;
	
fin:
	return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
// Code is running with preemtion disabled.
	return;
}

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
// Code is running with preemtion disabled.
	return 0;
}

static int __init init_kprobe(void)
{
    int ret;
    kp.pre_handler = handler_pre;
    kp.post_handler = handler_post;
    kp.fault_handler = handler_fault;

    ret = register_kprobe(&kp);

    if (ret < 0) 
    {
        pr_info("register_kprobe failed, returned %d\n", ret);
        return ret;
    }
    pr_info("Planted kprobe at %p\n", kp.addr);
    return ret;
}

static void __exit exit_kprobe(void)
{
	unregister_kprobe(&kp);
	pr_info("pmon module unloaded\n");
}


module_init(init_kprobe);
module_exit(exit_kprobe);

MODULE_LICENSE("GPL");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ksm.h>
#include <linux/rmap.h>
#include <linux/rcupdate.h>
#include <linux/export.h>
#include <linux/memcontrol.h>
#include <linux/mmu_notifier.h>
#include <linux/migrate.h>
#include <linux/hugetlb.h>

#include <asm/tlbflush.h>

int pid = 3115;

static void show_avs_by_vma(struct vm_area_struct *vma)
{
	//struct anon_vma *anon_vma;
	struct anon_vma_chain *pavc;

	list_for_each_entry_reverse(pavc, &vma->anon_vma_chain, same_vma) {
		printk(KERN_DEBUG "    |--- avc:%p, pavc->anon_vma:%p\n", pavc, pavc->anon_vma);
		if (pavc->vma != vma)
			printk(KERN_DEBUG "        NOTE: DIFF vma:%p, pavc->vma:%p\n", vma, pavc->vma);
		if (pavc->anon_vma)
			printk(KERN_DEBUG "        |--- anon_vma root:%p parent:%p, degree:%u, ref:%d",
				pavc->anon_vma->root,
				pavc->anon_vma->parent,
				pavc->anon_vma->degree,
				anonvma_external_refcount(pavc->anon_vma));
	}
}

static int show_vmas_by_pid(int pid)
{
	//int err;
	struct task_struct *tsk;
	struct pid *kpid;
	struct mm_struct *mm;
	struct vm_area_struct *vma = NULL;

	kpid = find_get_pid(pid);
	if (kpid == NULL)
		return -ENOMEM;

	tsk = pid_task(kpid, PIDTYPE_PID);
	mm = tsk->mm;
	if (mm)
		vma = mm->mmap;
	printk(KERN_DEBUG "PID:%d task:%p\n", pid, tsk);
	while (vma) {
		printk(KERN_DEBUG "|--- vma: %p, range[%lx - %lx] anon_vm: %p\n", vma,
			vma->vm_start, vma->vm_end, vma->anon_vma);
		show_avs_by_vma(vma);
		vma = vma->vm_next;
	}
	put_pid(kpid);
	return 0;
}

int __init tcp_hook_init(void)
{
	show_vmas_by_pid(pid);
	//show_vmas_by_pid(pid+1);

	return 0;
}

void __exit tcp_hook_exit(void)
{
	return;
}

module_init(tcp_hook_init);
module_exit(tcp_hook_exit);
MODULE_LICENSE("GPL");

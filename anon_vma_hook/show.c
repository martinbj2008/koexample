#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/kernel.h>
#include <asm/unaligned.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ksm.h>
#include <linux/rmap.h>
#include <linux/rcupdate.h>
#include <linux/module.h>
#include <linux/memcontrol.h>
#include <linux/mmu_notifier.h>
#include <linux/migrate.h>
#include <linux/hugetlb.h>
#include <linux/backing-dev.h>
#include <trace/events/kmem.h>
#include <asm/tlbflush.h>

/*
 *  * This is a useful helper function for locking the anon_vma root as
 *   * we traverse the vma->anon_vma_chain, looping over anon_vma's that
 *    * have the same vma.
 *     *
 *      * Such anon_vma's should have the same root, so you'd expect to see
 *       * just a single mutex_lock for the whole traversal.
 *        */
static inline struct anon_vma *lock_anon_vma_root(struct anon_vma *root, struct anon_vma *anon_vma)
{
        struct anon_vma *new_root = anon_vma->root;
        if (new_root != root) {
                if (WARN_ON_ONCE(root))
                        spin_unlock(&root->lock);
                root = new_root;
                spin_lock(&root->lock);
        }
        return root;
}

static inline void unlock_anon_vma_root(struct anon_vma *root)
{
        if (root)
                spin_unlock(&root->lock);
}

void __j_unlink_anon_vmas(struct vm_area_struct *vma)
{
	int found = 0;
        struct anon_vma_chain *avc, *next;
        struct anon_vma *root = NULL;
        struct anon_vma *anon_vma;

        /* Unlink each anon_vma chained to the VMA, from newest to oldest. */
        list_for_each_entry_safe(avc, next, &vma->anon_vma_chain, same_vma) {
		anon_vma = avc->anon_vma;
		root = lock_anon_vma_root(root, anon_vma);
                if (anonvma_external_refcount(anon_vma)) {
			//Only one avc
			if (anon_vma->head.next == anon_vma->head.prev 
				&& anon_vma->head.next == &avc->same_anon_vma) {
				found = 1;
				break;
			}
		}
        }
        unlock_anon_vma_root(root);
	if (found) {
		BUG_ON(anonvma_external_refcount(anon_vma));
	}

	jprobe_return();
	return;
}

static struct jprobe unlink_anon_vmas_jp = {
	.entry          = __j_unlink_anon_vmas,
	.kp.symbol_name = "unlink_anon_vmas"
};

static int __init kprobe_init(void)
{
	int ret;

	ret = register_jprobe(&unlink_anon_vmas_jp);
	if (ret == 0)
		printk(KERN_INFO "Register Jprobe for unlink_anon_vmas OK\n");

	return ret;
}

static void __exit kprobe_exit(void)
{
	unregister_jprobe(&unlink_anon_vmas_jp);
	printk(KERN_INFO "Unregister Jprobe for unlink_anon_vmas OK\n");
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");

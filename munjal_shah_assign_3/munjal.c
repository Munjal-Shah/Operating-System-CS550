#include <linux/init.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <asm/current.h>
#include <linux/cred.h>
#include <linux/mm_types.h>
#include <asm/pgtable.h>

MODULE_LICENSE("GPL");
static int process_id;
module_param(process_id, int, S_IRUGO);


/*
 * Called when module is installed
 */
int __init init_module() {

	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep,pte;

	unsigned long page_frame_number;

	struct pid *pid;
	struct task_struct *pid_struct;
	struct mm_struct *pid_mm_struct;
	struct vm_area_struct *vma;
	unsigned long vaddr;

	pid = find_get_pid(process_id);
	pid_struct = pid_task(pid,PIDTYPE_PID);
	pid_mm_struct = pid_struct->mm;

	for(vma = pid_mm_struct->mmap;vma; vma = vma->vm_next) {
		
		printk(KERN_ALERT "start address: %lx\n", vm_start);
		printk(KERN_ALERT "end address: %lx\n", vm_end);
		
		for(vaddr = vma->vm_start; vaddr<vma->vm_end; vaddr++) {
			
			pgd = pgd_offset(pid_mm_struct,vaddr);
			
			p4d = p4d_offset(pgd,vaddr);
			
			pud = pud_offset(p4d,vaddr);
			
			pmd = pmd_offset(pud,vaddr);
			
			ptep = pte_offset_map(pmd,vaddr);
			
			pte = *ptep;
			if (pte_present(pte)) {
				page_frame_number = pte_pfn(pte);
				printk(KERN_ALERT "Page Frame Number= %lx\n", page_frame_number);
			}
		}
	}

	printk(KERN_ALERT "mymodule: Process ID: %d!\n", process_id);

	return 0;
}


/*
 * Called when module is removed
 */
void __exit cleanup_module() {
	printk(KERN_ALERT "mymodule: Goodbye, Process ID: %d!!\n", process_id);
}
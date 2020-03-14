/*
 * @Author: your name
 * @Date: 1970-01-01 08:00:00
 * @LastEditTime: 2020-03-14 19:49:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rasp-driver/memory/mmap.c
 */




#include <linux/fs.h>
#include <linux/mm.h>

static void xxx_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE"xxx VMA opened, virt %lx, phys %lx\n", 
		vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}


static void xxx_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE"xxx VMA closed\n");
}


static struct vm_operations_struct xxx_remap_vm_ops = {
	.open = xxx_vma_open,
	.close = xxx_vma_close
};



static int xxx_mmap(struct file filp, struct vm_area_struct *vma)
{
	// 创建页表项，以VMA结构体的成员（VMA的数据成员是内核根据用户的请求自己填充的）
	// 作为参数， 虚拟地址范围vma->vm_start至vma->vm_end。
	// prot是新页所要求的保护属性。
	int ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, 
		vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (ret)
		return -EAGAIN;
	
	vma->vm_ops = &xxx_remap_vm_ops;
	// xxx_vma_open(vma);
	if (vma->vm_ops->open)
		vma->vm_ops->open(vma);

	return 0;
}






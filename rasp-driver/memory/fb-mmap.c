#include <linux/module.h>

#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>



static int fb_mmap(struct file *filp, struct vm_area_struct *vma)
{
	// 获取fb_info结构
	struct fb_info *info = file_fb_info(filp);
	struct fb_ops *fb;
	unsigned long mmio_pgoff;
	unsigned long start;
	u32 len;

	if (!info)
		return -ENODEV;
	fb = info->fbops;
	if (!fb)
		return -ENODEV;
	
	// 锁定内存并开始分配内存
	mutex_lock(&info->mm_lock);
	if (fb->fb_mmap) {
		int res;
		// 如果存在mmap函数则直接调用
		res = fb->fb_mmap(info, vma);
		mutex_unlock(&info->mm_lock);
		return res;
	}

	// 这可以是帧缓冲区映射，也可以是pgoff指向的地方，即mmio映射。
	start = info->fix.smem_start;
	len = info->fix.smem_len; 
	mmio_pgoff = PAGE_ALIGN((start & ~PAGE_MASK) + len) >> PAGE_SHIFT;
	if (vma->vm_pgoff >= mmio_pgoff) {
		if (info->var.accel_flags) {
			mutex_unlock(&info->mm_lock);
			return -EINVAL;
		}
	
		vma->vm_pgoff -= mmio_pgoff;
		start = info->fix.mmio_start;
		len = info->fix.mmio_len;
	}
	mutex_unlock(&info->mm_lock);

	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	fb_pgprotect(filp, vma, start);

	return vm_iomap_memory(vma, start, len);
}


/**
 * pgprot_noncached禁止了相关页的cache和写缓冲, 
 */
static int xxx_nochche_mmap(struct file *filp, struct vm_area_struct *vma)
{
	// 实现机制依赖与CPU体系结构, 设置nocache标志
	vma->vm_page_prot = pgprot_nocached(vma->vm_page_prot);
	vma->vm_pgoff = ((u32)map_start >> PAGE_SHIFT);

	if (remap_pfn_range(vma, vma_start, vma->vm_pgoff, 
		vma->vm_end - vma->vma_start, vma->vm_page_prot))
		return -EAGAIN;
}


/**
 * 当访问的页不在内存里，即发生缺页异常时，fault（）会被内核自动调用，
 * 而fault（）的具体行为可以自定义。缺页时候处理如下
 * 
 * 1）找到缺页的虚拟地址所在的VMA。
 * 2）如果必要，分配中间页目录表和页表。
 * 3）如果页表项对应的物理页面不存在，则调用这个VMA的fault（）方法，
 * 	它返回物理页面的页描迏符。
 * 4）将物理页面的地址填充到页表中。
 */
static int xxx_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	unsigned long paddr;
	unsigned long pfn;
	pgoff_t index = vmf->pgoff;
	struct vma_data *vdata = vma->vm_private_data;
	
	pass;

	pfn = paddr >> PAGE_SHIFT;
	vm_insert_pfn(vma, (unsigned long)vmf->virtual_address, pfn);
	return VM_FAULT_NOPAGE;
}


// 大多数设备驱动都不需要提供设备内存到用户空间的映射能力，
// 因为，对于串口等面向流的设备而言，实现这种映射毫无意义。
// 而对于显示、视频等设备，建立映射可减少用户空间和内核空间之间的内存复制。

/**
 * 当进行Linux移植时, 需要外设IO内存物理地址到虚拟地址的静态映射
 * 这个映射通过在与电路板对应的map_desc结构数组中添加新成员来完成
 */
struct map_desc {
	unsigned long virtual; // 虚拟地址
	unsigned long pfn; //__phys_to_pfn(phy_addr)
	unsigned long length; //大小
	unsigned int type; //类型
};


/**
 * 电路板文件添加物理到虚拟的静态映射
 */
static struct map_desc ixdp2x01_io_desc __initdata = {
	.virtual		= IXDP2X01_VIRT_CPLD_BASE,
	.pfn			= __phys_to_pfn(IXDP2X01_PHYS_CPLD_BASE),
	.length			= IXDP2X01_CPLD_REGION_SIZE,
	.type			= MT_DEVICE
};


static void __init ixdp2x01_map_io(void)
{
	ixp2000_map_io();
	// 建立页映射
	iotable_init(&ixdp2x01_io_desc, 1);
}







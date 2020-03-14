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
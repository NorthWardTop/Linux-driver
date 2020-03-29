

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
#include <linux/fbcon.h>
#include <linux/mem_encrypt.h>
#include <linux/pci.h>

#include <asm/fb.h>
#include <asm-generic/page.h>



static sszie_t fb_write(struct file *file, const char __user *buf, 
	size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	struct fb_info *info = file_fb_info(file);
	u8 *buffer, *src;
	u8 __iomem *dst;
	int c, cnt = 0, err = 0;
	unsigned long total_size;
	
	if (!info || !info->screen_base)
		return -ENODEV;
	
	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;
	
	/**检查底层LCD有没有实现自己特殊显存写法的代码，如果有，直接调底层的并返回
	 * 如果没有, 这个标准write函数将作为fb_write继续执行
	 * 这个函数相当于基类函数, 没有被重写就执行
	 */
	if (info->fbops->fb_write)
		return info->fbops->fb_write(info, buf, count, ppos);

	// 获取显存内存总大小
	total_size = info->screen_size;
	if (total_size == 0)
		total_size = info->fix.smem_len;

	// 参数合法性判断, 并尽量修正参数
	if (p > total_size) // 写指针超出范围, 直接返回
		return -EFBIG;
	if (count > total_size) { // 写的内容超出总大小, 修正为总大小
		err = -EFBIG;
		count = total_size;
	}
	if (count + p > total_size) { // 指针后移写内容后超出总大小, 修正为剩余空间
		if (!err)
			err = -ENOSPC;
		count = total_size - p;
	}

	// 分配不大于一页的缓冲区
	buffer = kmalloc((count > PAGE_SIZE ? PAGE_SIZE : count), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;
	
	dst = (u8 __iomem*)(info->screen_base + p);

	if (info->fbops->fb_sync)
		info->fbops->fb_sync(info);

	while (count) {
		c = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		src = buffer; //给src分配缓冲, src指向有空间

		// src = buf; 将应用层的buf数据复制到src指针(缓冲区内)
		if (copy_from_user(src, buf, c)) {
			err = -EFAULT;
			break;
		}

		// dst = src; 再从src(缓冲区)复制到frambuffer
		fb_memcpy_tofb(dst, src, c);
		dst += c;
		src += c;
		*ppos += c; //传回的数据, 文件指针
		buf += c; //用户数据指针后移
		cnt += c; // 写入计数
		count -= c; // 未写计数
	}

	kfree(buffer);

	return cnt ? cnt : err;
}
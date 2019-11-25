#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/file.h>
#include <linux/blkdev.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/percpu.h>
#include <linux/uio.h>
#include <linux/idr.h>
#include <linux/bsg.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>


#define LKM_NAME 		"lkm_memory"
#define LKM_MEM_SIZE  	1024


struct lkm_mem_dev
{
	u_char *name; //device name
	int major;
	int minor;
	struct file_operations *fops;
	struct class *mem_class;
	u_char *memory;
	ushort len; //current data length
};

static struct lkm_mem_dev *pmem_dev;


static int lkm_mem_open(struct inode *inode, struct file *filp)
{
	//注册对象: 将设备文件的私有数据域指向对象
	filp->private_data = pmem_dev;
	printk(KERN_INFO"lkm_mem_dev is opened\n");
	return 0;
}


static int lkm_mem_release(struct inode *ino, struct file *filp)
{
	filp->private_data = NULL;
	printk(KERN_INFO"lkm_mem_dev is closed\n");
	return 0;
}


static struct file_operations lkm_mem_fops = {
	.owner = THIS_MODULE,
	.open = lkm_mem_open,
	.release = lkm_mem_release,
	.read = lkm_mem_read,
	.write = lkm_mem_write,
};


static __init int lkm_mem_init(void)
{
	int ret;
	//分配对象
	pmem_dev = kzalloc(sizeof(struct lkm_mem_dev), GFP_KERNEL);
	if (!pmem_dev) {
		ret = -ENOMEM;
		printk(KERN_ERR"can't alloc device memory\n");
		goto handler_ealloc;
	}

	//设置对象名称
	pmem_dev->name = kzalloc(sizeof(LKM_NAME), GFP_KERNEL);
	if (!pmem_dev->name) {
		ret = -ENOMEM;
		printk(KERN_ERR"can't alloc name memory\n");
		goto handler_ealloc_name;
	}
	memcpy(pmem_dev->name, LKM_NAME, sizeof(LKM_NAME));
	//设置对象fops
	pmem_dev->fops = &lkm_mem_fops;
	pmem_dev->major = register_chrdev(0, pmem_dev->name, pmem_dev->fops);
	



	printk(KERN_INFO"module loaded\n");
	return 0;

handler_ealloc_name:
	kfree(pmem_dev);
handler_ealloc:
	return ret;
}



static __exit void lkm_mem_exit(void)
{
	printk("Chandler:module removed\n");

}




module_init(lkm_init);
module_exit(lkm_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yonghuilee.cn@gmail.com");
MODULE_DESCRIPTION("LKM first process");
MODULE_VERSION("1.0");










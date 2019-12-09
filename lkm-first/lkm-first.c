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
#include <linux/atomic.h>


#define LKM_NAME 		"lkm_memory"
#define LKM_MEM_SIZE  	1024


struct lkm_mem_dev {
	u_char *name; //device name
	int major;
	int minor;
	struct file_operations *fops;
	struct class *dev_class;
	u_char *memory;
	ushort len; //current data length
	//并发控制原子变量
	atomic_t open_count;
};

static struct lkm_mem_dev *pmem_dev;


static int lkm_mem_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	//注册对象: 将设备文件的私有数据域指向对象
	filp->private_data = pmem_dev;
	//并发控制, 测试并自减
	ret = atomic_dec_and_test(&pmem_dev->open_count);
	if (!ret) {
		printk("lkm_mem_dev busy\n");
		atomic_inc(&pmem_dev->open_count);
		ret = -EBUSY;
	} else 
		printk(KERN_INFO"lkm_mem_dev is opened\n");
	return ret;
}


static int lkm_mem_release(struct inode *ino, struct file *filp)
{
	struct lkm_mem_dev *dev = (struct lkm_mem_dev*)filp->private_data;
	filp->private_data = NULL;
	atomic_inc(&dev->open_count);
	printk(KERN_INFO"lkm_mem_dev is closed\n");
	return 0;
}


static ssize_t lkm_mem_read(struct file *filp, char __user *buf, size_t len, loff_t *pos)
{
	unsigned long ptmp = *pos;
	unsigned int count = len;
	int 		   ret = 0;
	struct lkm_mem_dev *dev = (struct lkm_mem_dev*)filp->private_data;

	printk("lkm_mem_read\n");
	if (ptmp == 0) 
		return 0;
	if (count > ptmp) //读取长度大于最大长度
		count = ptmp;
	ret = copy_to_user(buf, dev->memory+ptmp, count);
	if (ret != 0)
		ret = -EFAULT;
	else {
		*pos -= count;
		ret = count;
		printk("read %u complete!\n", count);
	}
	return ret;
}


static ssize_t lkm_mem_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos)
{
	unsigned long ptmp = *pos;
	unsigned int count = len;
	int 		   ret = 0;
	struct lkm_mem_dev *dev = (struct lkm_mem_dev*)filp->private_data;

	printk("lkm_mem_write\n");
	if (ptmp >= LKM_MEM_SIZE)
		return 0;
	if (count > LKM_MEM_SIZE - ptmp)
		count = LKM_MEM_SIZE - ptmp;
	ret = copy_from_user(dev->memory + ptmp, buf, count);
	if (ret != 0)
		ret = -EFAULT;
	else {
		*pos += count;
		ret = count;
		printk("lkm_mem_write\n");
	}
	return ret;
}


static struct file_operations lkm_mem_fops = {
	.owner = THIS_MODULE,
	.open = lkm_mem_open,
	.release = lkm_mem_release,
	.read = lkm_mem_read,
	.write = lkm_mem_write,
};

static int setup_dev(struct lkm_mem_dev* dev, int index)
{
	dev->minor = index; //设置具体设备号, 次设备号
	memset(dev->memory, 0, LKM_MEM_SIZE);
	dev->len = 0;
	atomic_inc(&pmem_dev->open_count);
	return true;
}


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
	//注册字符设备
	pmem_dev->major = register_chrdev(0, pmem_dev->name, pmem_dev->fops);
	if (pmem_dev->major < 0) {
		ret = pmem_dev->major;
		printk("can't register char device\n");
		goto handler_eregister;
	}

	ret = setup_dev(pmem_dev, 0);
	if (!ret) {
		ret = -EINVAL;
		printk("setup devices failed\n");
		goto handler_esetup;
	}
	//创建设备类
	pmem_dev->dev_class = class_create(THIS_MODULE, pmem_dev->name);
	if (IS_ERR(pmem_dev->dev_class)) {
		ret = PTR_ERR(pmem_dev->dev_class);
		printk("can't create device class\n");
		goto handler_eclass_create;
	}
	//创建设备实体
	device_create(pmem_dev->dev_class, NULL, MKDEV(pmem_dev->major, pmem_dev->minor), \
		NULL, pmem_dev->name);
	
	printk(KERN_INFO"module loaded\n");
	return 0;



handler_eclass_create:
handler_esetup:
	unregister_chrdev(pmem_dev->major, pmem_dev->name);
handler_eregister:
	kfree(pmem_dev->name);
handler_ealloc_name:
	kfree(pmem_dev);
handler_ealloc:
	return ret;
}



static __exit void lkm_mem_exit(void)
{
	device_destroy(pmem_dev->dev_class, MKDEV(pmem_dev->major, pmem_dev->minor));
	class_destroy(pmem_dev->dev_class);
	unregister_chrdev(pmem_dev->major, pmem_dev->name);
	kfree(pmem_dev->name);
	kfree(pmem_dev);
	printk("module removed\n");
}




module_init(lkm_mem_init);
module_exit(lkm_mem_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yonghuilee.cn@gmail.com");
MODULE_DESCRIPTION("LKM first process");
MODULE_VERSION("1.0");



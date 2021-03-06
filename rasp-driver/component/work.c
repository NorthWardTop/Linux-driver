
/**
 * 测试poll系统调用的module
 */


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
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/workqueue.h>

#include <asm/current.h>

#define LKM_NAME 		"lkm_memory"
#define LKM_MEM_SIZE  	1024


// /usr/src/linux-headers-4.15.0-30deepin/**
// /media/lee/_dde_data/LinuxSrc/raspbian/include/**

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
	//自旋锁
	spinlock_t slock;
	struct mutex mutex;
	//等待队列
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
	//异步队列
	struct fasync_struct *fasync;
	//工作队列结构体
	struct work_struct work;
};

//设备对象指针
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
	} else {
		printk(KERN_INFO"lkm_mem_dev is opened\n");
		ret = 0;
	}
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
	// unsigned long ptmp = *pos;
	unsigned int count = len;
	int 		   ret = 0;
	struct lkm_mem_dev *dev = (struct lkm_mem_dev*)filp->private_data;

	printk("lkm_mem_read\n");
	DECLARE_WAITQUEUE(read_wait, current); //定义等待队列的一个节点
	mutex_lock(&dev->mutex); //操作前枷锁
	add_wait_queue(&dev->r_wait, &read_wait);
	//判断当前fifo是否可读
	while (dev->len == 0) {
		//阻塞前判断当前fifo是否为阻塞模式, 否则立即返回
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto exit;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);
		schedule();
		//是否为中断唤醒
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto exit_2;
		}
		//有资源可用唤醒
		__set_current_state(TASK_RUNNING);
		mutex_lock(&dev->mutex);
	}


	if (count > dev->len) //读取长度大于最大长度
		count = dev->len;
	ret = copy_to_user(buf, dev->memory, count);
	if (ret != 0) {
		ret = -EFAULT;
		goto exit_2;
	} else {
		//向前更新fifo
		memcpy(dev->memory, dev->memory + count, dev->len - count);
		dev->len -= count;
		ret = count;
		printk("read %u complete!\n", count);
		wake_up_interruptible(&dev->w_wait); //唤醒写
		if (dev->fasync) {
			kill_fasync(&dev->fasync, SIGUSR2, POLL_OUT);
			printk("%s kill SIGUSR2\n", __func__);
		}
	}

exit_2:
	__set_current_state(TASK_RUNNING);

exit:
	remove_wait_queue(&dev->r_wait, &read_wait);
	mutex_unlock(&dev->mutex);
	return ret;
}


static ssize_t lkm_mem_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos)
{
	unsigned int count = len;
	int 		   ret = 0;
	struct lkm_mem_dev *dev = (struct lkm_mem_dev*)filp->private_data;
	printk("lkm_mem_write\n");

	DECLARE_WAITQUEUE(write_wait, current); //定义等待队列的一个节点
	mutex_lock(&dev->mutex); //操作前枷锁
	add_wait_queue(&dev->w_wait, &write_wait);
	//判断当前fifo是否可读
	while (dev->len == LKM_MEM_SIZE) {
		if (dev->fasync) {
			kill_fasync(&dev->fasync, SIGUSR1, POLL_HUP);
			printk("%s kill SIGUSR1\n",__func__);
		}

		//阻塞前判断当前fifo是否为阻塞模式, 否则立即返回
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto exit;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);
		schedule();
		//是否为中断唤醒
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto exit_2;
		}
		//有资源可用唤醒
		__set_current_state(TASK_RUNNING);
		mutex_lock(&dev->mutex);
	}


	if (count > LKM_MEM_SIZE - dev->len)
		count = LKM_MEM_SIZE - dev->len;
	ret = copy_from_user(dev->memory + dev->len, buf, count);
	if (ret != 0) {
		ret = -EFAULT;
		goto exit_2;
	} else {
		dev->len += count;
		ret = count;
		printk("lkm_mem_write\n");
		wake_up_interruptible(&dev->r_wait);
		if (dev->fasync) {
			kill_fasync(&dev->fasync, SIGIO, POLL_IN);
			printk("%s kill SIGIO\n", __func__);
		}
	}


exit_2:
	__set_current_state(TASK_RUNNING);

exit:
	remove_wait_queue(&dev->w_wait, &write_wait);
	mutex_unlock(&dev->mutex);
	return ret;
}


static unsigned int lkm_mem_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int imask=0;
	//从filp获得设备结构体指针
	struct lkm_mem_dev *dev = (struct lkm_mem_dev*)filp->private_data;
	mutex_lock(&dev->mutex);
	// 当前进程加入poll table读队列
	poll_wait(filp, &dev->r_wait, wait);
	poll_wait(filp, &dev->w_wait, wait);

	if (dev->len != 0) {
		imask |= POLLIN | POLLRDNORM;
	if (dev->len != LKM_MEM_SIZE)
		imask |= POLLOUT | POLLRDNORM;
	}
	printk("poll finish\n");

	mutex_unlock(&dev->mutex);

	return imask;
}


static int lkm_mem_fasync(int fd, struct file *filp, int mode)
{
	struct lkm_mem_dev *devp;

	devp = filp->private_data;

	//将当前文件加入fasync链表
	if (fasync_helper(fd, filp, mode, &devp->fasync) >= 0)
		return 0;
	else
		return -EIO;
}


static struct file_operations lkm_mem_fops = {
	.owner = THIS_MODULE,
	.open = lkm_mem_open,
	.release = lkm_mem_release,
	.read = lkm_mem_read,
	.write = lkm_mem_write,
	.poll = lkm_mem_poll,
	.fasync = lkm_mem_fasync,
};


static int do_work(void *arg)
{
	printk("pid: %d, new kernel thread running...", current->pid);
}

static void init_work(struct work_struct *arg)
{
	struct lkm_mem_dev* dev = container_of(arg, struct lkm_mem_dev, work);
	int ret = kernel_thread(do_work, dev, \
		CLONE_FS | CLONE_PARENT | CLONE_SIGHAND | CLONE_FILES);

	printk("current pid: %d", current->pid);

}


static int setup_dev(struct lkm_mem_dev* dev, int index)
{
	dev->minor = index; //设置具体设备号, 次设备号

	dev->memory = kzalloc(LKM_MEM_SIZE, GFP_KERNEL);
	memset(dev->memory, 0, LKM_MEM_SIZE);
	dev->len = 0;
	//可打开计数初始化位1, 表示可以被同时打开一次
	atomic_set(&dev->open_count, 1); 
	spin_lock_init(&dev->slock);
	mutex_init(&dev->mutex);
	
	//等待队列初始化
	init_waitqueue_head(&dev->r_wait);
	init_waitqueue_head(&dev->w_wait);

	//初始化异步队列
	dev->fasync = NULL;

	INIT_WORK(&dev->work, inti_work);

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



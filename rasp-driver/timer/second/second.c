#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/errno.h>


#define SECOND_MAJOR		260 //248

static int second_major = SECOND_MAJOR;
module_param(second_major, int, S_IRUGO);

struct second_dev {
	struct cdev cdev;
	atomic_t counter;
	struct timer_list timer;
};

static struct second_dev *second_devp;

static void second_timer_handler(struct timer_list *timer)
{
	mod_timer(&second_devp->timer, jiffies + HZ); //设定下一次触发时间
	atomic_inc(&second_devp->counter);
	printk(KERN_INFO "Current jiffies is %ld\n", jiffies);
}

//打开秒设备
static int second_open(struct inode *inode, struct file *filp)
{
	//过时, kernel 4+已丢弃
	// init_timer(&second_devp->timer); //初始化定时器
	//second_devp->timer.function = &second_timer_handler;

	//设置对象
	timer_setup(&second_devp->timer,second_timer_handler,0);
	second_devp->timer.expires = jiffies + HZ;
	//注册对象
	add_timer(&second_devp->timer);

	atomic_set(&second_devp->counter, 0);
	return 0;
}


//释放秒设备
static int second_release(struct inode *inode, struct file *filp)
{
	if (del_timer(&second_devp->timer) < 0) {
		printk(KERN_ERR "delete timer failed\n");
		return -EFAULT;
	}
	return 0;
}


//应用层读取counter
static ssize_t second_read(struct file *filp, char __user *buf, size_t count,
	loff_t *ppos)
{
	int counter;
	counter = atomic_read(&second_devp->counter);
	if (put_user(counter, (int*)buf))
		return -EFAULT;
	else 
		return sizeof(unsigned int);
}



static const struct file_operations second_fops = {
	.owner = THIS_MODULE,
	.open = second_open,
	.release = second_release,
	.read = second_read,
};


static void second_setup_cdev(struct second_dev *dev, int index)
{
	int err, devno = MKDEV(second_major, index);
	cdev_init(&dev->cdev, &second_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_ERR "Failed to add second device\n");
}

static int __init second_init(void)
{
	int ret = 0;
	dev_t devno = MKDEV(second_major, 0);

	if (devno) //预计代码的BUG
		ret = register_chrdev_region(devno, 1, "second");
	else {
		ret = alloc_chrdev_region(&devno, 0, 1, "second");
		second_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;

	second_devp = kzalloc(sizeof(*second_devp), GFP_KERNEL);
	if (!second_devp) {
		ret = -ENOMEM;
		goto fail_malloc;
	}

	second_setup_cdev(second_devp, 0);
	return 0;

fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
}



static void __exit second_exit(void)
{
	cdev_del(&second_devp->cdev);
	kfree(second_devp);
	unregister_chrdev_region(MKDEV(second_major, 0), 1);
}


module_init(second_init);
module_exit(second_exit);

MODULE_AUTHOR("2394783277@qq.com");
MODULE_LICENSE("GPL v2");




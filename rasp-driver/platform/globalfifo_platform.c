

/**
 * globalfifo驱动挂接到platform总线上，这要完成两个工作。
 * 1）将globalfifo移植为platform驱动。
 * 2）在板文件中添加globalfifo这个platform设备。
 * 为完成将globalfifo移植到platform驱动的工作，
 * 需要在原始的globalfifo字符设备驱动中套一层platform_driver的外壳，
 * 如代码清单12.6所示。注意进行这一工作后，并没有改变globalfifo是字符设备的本质，
 * 只是将其挂接到了platform总线上。
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

#define LKM_NAME 		"lkm_memory"
#define LKM_MEM_SIZE  	1024



struct globalfifo_dev {
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
};

struct globalfifo_dev *globalfifo_devp;

static int globalfifo_probe(struct platform_device *pdev)
{
	int ret;
	dev_t devno = MKDEV(globalfifo_major, 0);

	if (globalfifo_major)
		ret = register_chrdev_region(devno, 1, "globalfifo");
	else {
		ret = alloc_chrdev_region(&devno, 0, 1, "globalfifo");
		globalfifo_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;

	globalfifo_devp = devm_kzalloc(&pdev->dev, sizeof(*globalfifo_devp), 
		GFP_KERNEL);
	
	if (!globalfifo_devp) {
		ret = -ENOMEM;
		goto fail_malloc;
	}

	globalfifo_setup_cdev(globalfifo_devp, 0);

	mutex_init(&globalfifo_devp->mutex);
	init_waitqueue_head(&globalfifo_devp);
	init_waitqueue_head(&globalfifo_devp);

	return 0;

fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
}


static int globalfifo_remove(struct platform_device *pdev)
{
	cdev_del(&globalfifo_devp->cdev);
	unregister_chrdev_region(MKDEV(globalfifo_major, 0), 1);

	return 0;
}

static struct platform_driver globalfifo_driver = {
	.driver = {
		.name = "globalfifo",
		.owner = THIS_MODULE,
	},
	.probe = globalfifo_probe,
	.remove = globalfifo_remove,
};

module_platform_driver(globalfifo_driver);




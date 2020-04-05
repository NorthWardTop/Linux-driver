
/**
 * 这是使用platform驱动框架, 使用杂项设备注册的设备
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>

#include <linux/sched/signal.h>


#define GLOBALFIFO_SIZE		0x1000
#define FIFO_CLEAR		0x1
#define GLOBALFIFO_MAJOR	231

/**
 * 字符设备驱动定义
 */
struct globalfifo_dev {
	struct cdev cdev;
	unsigned int current_len;
	unsigned char mem[GLOBALFIFO_SIZE];
	struct mutex mutex;
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
	struct fasync_struct *async_queue;
	struct miscdevice miscdev;
};


/**
 * 实现异步
 */
static int globalfifo_fasync(int fd, struct file *filp, int mode)
{
	// 通过对象的子对象地址, 反解出对象地址
	struct globalfifo_dev *dev = container_of(filp->private_data, 
		struct globalfifo_dev, miscdev);

	return fasync_helper(fd, filp, mode, &dev->async_queue);
}


static int globalfifo_open(struct inode *inode, struct file *filp)
{
	return 0;
}


static int globalfifo_release(struct inode *inode, struct file *filp)
{
	globalfifo_fasync(-1, filp, 0);
	return 0;
}


static long globalfifo_ioctl(struct file *filp, unsigned int cmd, 
				unsigned long arg)
{
	struct globalfifo_dev *dev = container_of(filp->private_data, 
		struct globalfifo_dev, miscdev);
	
	switch (cmd) {
	case FIFO_CLEAR:
		mutex_lock(&dev->mutex);
		dev->current_len = 0;
		memset(dev->mem, 0, GLOBALFIFO_SIZE);
		mutex_unlock(&dev->mutex);

		printk(KERN_INFO "globalfifo is set to zero");
		break;
	
	default:
		return -EINVAL;
	}
	return 0;
}


static unsigned int globalfifo_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct globalfifo_dev *dev = container_of(filp->private_data, 
		struct globalfifo_dev, miscdev);
	
	mutex_lock(&dev->mutex);

	// 读写队列加入到等待队列
	poll_wait(filp, &dev->r_wait, wait);
	poll_wait(filp, &dev->w_wait, wait);

	//设置mask为可随机读
	if (dev->current_len != 0) {
		mask |= POLLIN | POLLRDNORM;
	}
	//设置mask为可随机写
	if (dev->current_len != GLOBALFIFO_SIZE) {
		mask |= POLLOUT | POLLWRNORM;
	}

	mutex_unlock(&dev->mutex);
	return mask;
}


static ssize_t globalfifo_read(struct file *filp, char __user *buf, 
				size_t count, loff_t *ppos)
{
	int ret;
	size_t len = count;
	struct globalfifo_dev *dev = container_of(filp->private_data, 
		struct globalfifo_dev, miscdev);

	// 给当前进程定义一个等待队列节点
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);
	// 添加读等待队列节点
	add_wait_queue(&dev->r_wait, &wait);

	// 在应用读取数据时候, 如果内容一直空则一直循环, 直到有数据可读
	while (dev->current_len == 0) {
		// flags设置为非阻塞则直接出去
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out; //需要解锁, 退到out
		}
		// 设置当前进程为可中断睡眠状态去等待
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex); // 解锁后调度, 否则其他进行无法访问设备
		schedule();

		// 调度运行其他进程后, 将从这句先执行
		// 判断到如果是有信号则返回并重新执行系统调用
		// 既: 如果是信号唤起, 则重新执行系统调用
		// 注意: 此时刚调度回来没有锁定
		if (signal_pending(current)) {
			ret = -ERESTARTSYS; // restart system
			goto out2; //不需要解锁, 退到out2
		}

		mutex_lock(&dev->mutex);
	}
	 
	//代码执行到这里, 一定是有数据可读并且处于锁定状态
	
	// 修正读长度参数
	if (len > dev->current_len)
		len = dev->current_len;
	
	if (copy_to_user(buf, dev->mem, len)) {
		ret = -EFAULT;
		goto out; //需要解锁, 退到out
	} else {
		// 读取成功后, 数据前移覆盖已读数据
		memcpy(dev->mem, dev->mem + len, dev->current_len - len);
		dev->current_len -= len;
		printk(KERN_INFO "read %d byte%c, current_len: %d\n", 
			len, len > 1 ? 's' : ' ', dev->current_len);
		
		// 唤醒写队列上阻塞的进程。
		// 该函数不能直接的立即唤醒进程，而是由调度程序转换上下文，调整为可运行状态。
		wake_up_interruptible(&dev->w_wait);
		ret = len;		
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->r_wait, &wait); // 任务完成从读等待队列移除
	set_current_state(TASK_RUNNING);
	return ret;
}


static ssize_t globalfifo_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int ret;
	size_t len = count;
	struct globalfifo_dev *dev = container_of(filp->private_data, 
		struct globalfifo_dev, miscdev);

	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->w_wait, &wait);

	// 是满的就一直循环
	while (dev->current_len == GLOBALFIFO_SIZE) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);
		schedule();

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}
		mutex_lock(&dev->mutex);
	}

	// 写入数据大于剩余空间
	if (len > GLOBALFIFO_SIZE - dev->current_len)
		len = GLOBALFIFO_SIZE - dev->current_len;

	if (copy_from_user(dev->mem + dev->current_len, buf, len)) {
		ret = -EFAULT;
		goto out;
	} else {
		dev->current_len += len;
		printk(KERN_INFO "written %d byte%c, current_len: %d\n", 
			len, len > 1 ? 's' : ' ', dev->current_len);
		
		wake_up_interruptible(&dev->r_wait); // 唤醒无数据可读阻塞的进程

		if (dev->async_queue) {
			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
			printk(KERN_DEBUG "%s kill SIGIO\n", __func__);
		}
		ret = len;
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return 0;
}


static const struct file_operations globalfifo_fops = {
	.owner		= THIS_MODULE,
	.open		= globalfifo_open,
	.release	= globalfifo_release,
	.read		= globalfifo_read,
	.write		= globalfifo_write,

	.unlocked_ioctl	= globalfifo_ioctl,
	.poll		= globalfifo_poll,
	.fasync		= globalfifo_fasync,
};


static int globalfifo_probe(struct platform_device *pdev)
{
	struct globalfifo_dev *gl;
	int ret;

	gl = devm_kzalloc(&pdev->dev, sizeof(*gl), GFP_KERNEL);
	if (!gl) 
		return -ENOMEM;
	
	// 设置对象
	gl->miscdev.minor = MISC_DYNAMIC_MINOR;
	gl->miscdev.name = "globalfifo";
	gl->miscdev.fops = &globalfifo_fops;

	mutex_init(&gl->mutex);
	init_waitqueue_head(&gl->r_wait);
	init_waitqueue_head(&gl->w_wait);
	platform_set_drvdata(pdev, gl); // 设置数据域反解结构体

	// 注册杂项设备
	ret = misc_register(&gl->miscdev);
	if (ret < 0)
		goto err;
	
	dev_info(&pdev->dev, "globalfifo driver probed\n");
	return 0;

err:
	return ret;
}


static int globalfifo_remove(struct platform_device *pdev)
{
	struct globalfifo_dev *gl = platform_get_drvdata(pdev);

	misc_deregister(&gl->miscdev);
	dev_info(&pdev->dev, "globalfifo driver removeed\n");

	return 0;
}


static struct platform_driver globalfifo_driver = {
	.driver = {
		.name = "globalfifo",
		.owner = THIS_MODULE,
	},
	.probe = globalfifo_probe,
	.remove = globalfifo_remove
};
module_platform_driver(globalfifo_driver);


MODULE_AUTHOR("yonghui <yonghuilee.cn@gmail.com>");
MODULE_LICENSE("GPL v2");


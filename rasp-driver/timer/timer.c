/**
 * timer
 */
#include <linux/fs.h>
#include <linux/timer.h>

struct xxx_dev
{
	struct cdev cdev;
	struct timer_list timer;
};


void xxx_do_timer(unsigned long arg, int delay)
{
	struct xxx_dev *dev = (struct xxx_dev*)arg;

	// 执行操作

	dev->timer.expires = jiffies + delay;
	add_timer(&dev->timer);
}

void reg_timer(struct file *filp, int delay)
{
	struct xxx_dev *dev = filp->private_data;
	dev->timer.function = xxx_do_timer;
	dev->timer.expires = jiffies + delay;
	//注册对象
	add_timer(&dev->timer);
}

void remove_timer(struct file *filp)
{
	struct xxx_dev *dev = filp->private_data;
	del_timer(&dev->timer);
}




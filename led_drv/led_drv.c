
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>



#define DEVICE_NAME		"LED"

static struct cdev gec6818_led_cdev;
static dev_t led_num=0;



static ssize_t gec6818_led_write(struct file *fl, const char __user *buf, size_t len, loff_t *off)
{
	printk("LED device write\n");
	return len;
}

//打开
static int gec6818_led_open(struct inode *i, struct file *f)
{
	printk("LED device opened!\n");
	return 0;
}

static int gec6818_led_release(struct inode *i, struct file *f)
{
	printk("LED closed\n");
	return 0;
}

static const struct file_operations gec6818_led_fops={
	.owner		= THIS_MODULE,
	.write		= gec6818_led_write,
	.open		= gec6818_led_open,
	.release 	= gec6818_led_release
};

static int __init gec6818_led_init(void)
{
	printk("GEC6818 LED installed, (c) 2019 SLsiAP\n");
	led_num=MKDEV(239, 0); //20位主设备号,12位次设备号,生成设备号
	int rt=register_chrdev_region(led_num, 1, DEVICE_NAME);//注册设备号, 数量, 名称
	if(rt < 0)
	{
		printk("register_chrdev_region fail\n");
		return rt;
	}
	
	//字符设备初始化
	cdev_init(&gec6818_led_cdev, &gec6818_led_fops);
	//加入内核
	rt=cdev_add(&gec6818_led_cdev, led_num, 1);
	if(rt<0)
	{
		printk("cdev_add failed!\n");
		goto unreg;
		return rt;
	}
	
	return 0;
		
unreg:
	unregister_chrdev_region(led_num, 1);
	return rt;

}

static void __exit gec6818_led_exit(void)
{
	cdev_del(&gec6818_led_cdev); //删除字符设备
	//注销字符设备号
	unregister_chrdev_region(led_num,1);
	printk("GEC6818 LED removed, (c) 2019 SLsiAP\n");

}

module_init(gec6818_led_init);
module_exit(gec6818_led_exit);

MODULE_AUTHOR("yonghuilee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");
/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: insmod和rmmod自动创建和销毁设备节点
 * "/dev/LED", "/sys/class/LED"
 * @Date: 2019-04-29 19:35:08
 * @LastEditTime: 2019-04-29 22:04:54
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>



#define CLASS_NAME		"LED"
#define DEVICE_NAME		"LED0"


static struct cdev gec6818_led_cdev;
static struct class *led_class;
static struct device *led_device;
static dev_t led_num = 0;


/**
 * @description: 当应用调用write函数时候, 将buf数据写入内核空间
 * 		copy_from_user(kbuf,buf,len);
 * @param {type}
 * @return:
 */
static ssize_t gec6818_led_write(struct file *fl, const char __user *buf, size_t len, loff_t *off)
{
	int ret = -1;
	char kbuf[64] = {};
	printk("LED device writing...\n");
	if (len > sizeof kbuf) //如果内核空间小装不下用户空间参数则报错
		return -EINVAL;//参数
	ret = copy_from_user(kbuf, buf, len);//返回cp字节数
	if (ret < 0)
	{
		printk("copy_from_user error\n");
		return -1;
	}
	printk("user say: %s\n", kbuf);

	return len - ret;
}

/**
 * @description: read函数, 数据从内核到应用层, copy_to_user();
 * @param {type}
 * @return:
 */
static ssize_t gec6818_led_read(struct file *fl, char __user *buf, size_t len, loff_t *offs)
{
	int ret = -1;
	char kbuf[64] = "I am from kernel data\n";
	printk("LED device reading...\n");
	if (len > sizeof kbuf) //如果内核空间小装不下用户空间参数则报错
		return -EINVAL;//参数
	ret = copy_to_user(buf, kbuf, len);
	if (ret < 0)
	{
		printk("copy_to_user error\n");
		return -1;
	}

	return len - ret;
}

//打开
static int gec6818_led_open(struct inode *i, struct file *f)
{
	printk("LED device opened!\n");
	return 0;
}


//关闭
static int gec6818_led_release(struct inode *i, struct file *f)
{
	printk("LED closed\n");
	return 0;
}

static const struct file_operations gec6818_led_fops = {
	.owner = THIS_MODULE,
	.write = gec6818_led_write,
	.read = gec6818_led_read,
	.open = gec6818_led_open,
	.release = gec6818_led_release
};



static int __init gec6818_led_init(void)
{
	printk("GEC6818 LED installed, (c) 2019 SLsiAP\n");
	//led_num=MKDEV(239, 0); //20位主设备号,12位次设备号,生成设备号
	//int rt=register_chrdev_region(led_num, 1, DEVICE_NAME);//注册设备号, 数量, 名称
	//防止同名设备号, 动态分配
	int ret = alloc_chrdev_region(&led_num, 0, 1, DEVICE_NAME);
	printk("led_major:%d,led_minor:%d\n", MAJOR(led_num), MINOR(led_num));

	if (ret < 0)
	{
		printk("register_chrdev_region fail\n");
		return ret;
	}

	//字符设备初始化
	cdev_init(&gec6818_led_cdev, &gec6818_led_fops);
	//加入内核
	ret = cdev_add(&gec6818_led_cdev, led_num, 1);
	if (ret < 0)
	{
		printk("cdev_add failed!\n");
		goto cdev_add_failed;
	}

	//在sys/class目录创建文件夹gec6818_leds
	//THIS_MODULE:class 的所有者，默认写 THIS_MODULE
	//class_led:自定义 class 的名字，会显示在/sys/class 目录当中
	led_class = class_create(THIS_MODULE, CLASS_NAME);//创建类
	if (IS_ERR(led_class))
	{
		ret = PTR_ERR(led_class);
		printk("class_create failed\n");
		goto class_create_failed;
	}
	//创建设备 
	/*
		struct  device  *device_create(struct  class  *class,  struct  device  *parent,
		dev_t  devt,  void  *drvdata,  const  char  *fmt,  ...)
		class：创建 device 是属于哪个类
		parent：默认为 NULL
		devt：设备号,设备号必须正确，因为这个函数会在/dev 目录下帮我们自动创建设备文件
		drvdata：默认为 NULL
		fmt：设备的名字，如果创建成功，就可以在/dev 目录看到该设备的名字 */
	led_device = device_create(led_class, NULL, led_num, NULL, DEVICE_NAME);
	if (IS_ERR(led_device))
	{
		ret = PTR_ERR(led_device);
		printk("device_create failed\n");
		goto device_create_failed;
	}
	printk("LED device initialization complete\n");

	return 0;

device_create_failed:
	class_destroy(led_class);

class_create_failed:
	cdev_del(&gec6818_led_cdev);

cdev_add_failed:
	unregister_chrdev_region(led_num, 1);

	return ret;

}

static void __exit gec6818_led_exit(void)
{
	//销毁设备
	device_destroy(led_class, led_num);
	//销毁类
	class_destroy(led_class);
	//内核中删除字符设备
	cdev_del(&gec6818_led_cdev);
	//注销字符设备号
	unregister_chrdev_region(led_num, 1);
	printk("GEC6818 LED removed, (c) 2019 SLsiAP\n");
}

module_init(gec6818_led_init);
module_exit(gec6818_led_exit);

MODULE_AUTHOR("yonghuilee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");


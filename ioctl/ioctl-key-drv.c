/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 本文件为KEY驱动程序
 * 使用ioctl实现应用层指令控制驱动程序
 * 实现: 按下K点亮LED
 * 驱动层:
 * 		注册一个杂项设备, 解析ioctl的指令, 
 * 		并读K值(copy_to_user)或写LED值(copy_from_user)
 * 应用层:
 * 		打开设备LED和K, 使用ioctl执行指令去读K设备值或写LED设备值
 *		
 * @Date: 2019-05-01 15:47:55
 * @LastEditTime: 2019-05-09 18:52:24
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <cfg_type.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include "key-led.h"


#define DEVICE_NAME				"KEYs"

struct key_gpio_info{
	unsigned int num;
	char name[16];
};

static const struct key_gpio_info key_info_tab[4]={
	{.num=PAD_GPIO_A+28, .name="gpioa28"},
	{.num=PAD_GPIO_B+9, .name="gpiob9"},
	{.num=PAD_GPIO_B+30, .name="gpiob30"},
	{.num=PAD_GPIO_B+31, .name="gpiob31"},
};


long gec6818_key_ioctl(struct file *fl, unsigned int cmd, unsigned long arg)
{
	//获取当前KEY灯下标
	unsigned long val=0;
	int i=0,ret=-1;
	switch(cmd)
	{
		case GEC6818_KALL_STAT:
			for(i=0;i<4;++i) //k2 k3 k4 k5
				val |= (gpio_get_value(key_info_tab[i].num)?0:1)<<i;

		case GEC6818_KEY2_STAT:
			val=gpio_get_value(key_info_tab[0].num)?0:1;
			break;
		case GEC6818_KEY3_STAT:
			val=gpio_get_value(key_info_tab[1].num)?0:1;
			break;
		case GEC6818_KEY4_STAT:
			val=gpio_get_value(key_info_tab[2].num)?0:1;
			break;
		case GEC6818_KEY5_STAT:
			val=gpio_get_value(key_info_tab[3].num)?0:1;
			break;
		default:
			return -ENOIOCTLCMD;
	}

	ret=copy_to_user((void*)arg, &val, sizeof(unsigned long));
	if(ret<0)
		return -EFAULT;

	return 0;
}



/**
 * @description: 当应用调用write函数时候, 将buf数据写入内核空间
 * 		copy_from_user(kbuf,buf,len);
 * @param {type} 
 * @return: 
 */
static ssize_t gec6818_key_write(struct file *fl, const char __user *buf, size_t len, loff_t *off)
{
	int ret=-1;
	char kbuf[64]="";
	printk("KEY device writing...\n");
	if(len>sizeof kbuf) //如果内核空间小装不下用户空间参数则报错
		return -EINVAL;//参数
	ret=copy_from_user(kbuf, buf, len);//返回cp字节数
	if(ret<0)
	{
		printk("copy_from_user error\n");
		return -1;
	}
	printk("user say: %s\n", kbuf);

	return len-ret;
}

/**
 * @description: read函数, 数据从内核到应用层, copy_to_user();
 * @param {type} 
 * @return: 
 */
static ssize_t gec6818_key_read(struct file *fl, char __user *buf, size_t len, loff_t *offs)
{
	int ret=-1;
	char kbuf[64]="I am from kernel data\n";
	printk("KEY device reading...\n");
	if(len>sizeof kbuf) //如果内核空间小装不下用户空间参数则报错
		return -EINVAL;//参数
	ret=copy_to_user(buf, kbuf, len);
	if(ret<0)
	{
		printk("copy_to_user error\n");
		return -1;
	}
	
	return len-ret;
} 

//打开
static int gec6818_key_open(struct inode *ino, struct file *f)
{
	int i=0;
	//配置对应gpio为输如模式
	for(;i<4;++i)
		gpio_direction_input(key_info_tab[i].num);
	printk("KEY device opened!\n");
	return 0;
}

//关闭
static int gec6818_key_release(struct inode *i, struct file *f)
{
	printk("KEY closed\n");
	return 0;
}

static const struct file_operations gec6818_key_fops={
	.owner		= THIS_MODULE,
	.write		= gec6818_key_write,
	.read 		= gec6818_key_read,
	.open		= gec6818_key_open,
	.release 	= gec6818_key_release,
	.unlocked_ioctl = gec6818_key_ioctl
};

static struct miscdevice gec6818_key_miscdev={
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVICE_NAME,
	.fops		= &gec6818_key_fops
};

static int __init gec6818_key_init(void)
{
	int i=0,ret=-1;
	//释放将要使用的gpio
	for(i=0;i<4;++i)
		gpio_free(key_info_tab[i].num);
	//申请gpio
	for(i=0;i<4;++i)
		ret=gpio_request(key_info_tab[i].num, key_info_tab[i].name);
	
	if(ret<0)
		goto gpio_request_failed;

	ret=misc_register(&gec6818_key_miscdev);
	if(ret<0)
		goto register_failed;
	
	printk("installed key_drv\n");
register_failed:
	printk("misc_register failed\n");
	misc_deregister(&gec6818_key_miscdev);

gpio_request_failed:
	printk("gpio_request failed\n");
	for(i=0;i<4;++i)
		gpio_free(key_info_tab[i].num);

	return ret;
}

static void __exit gec6818_key_exit(void)
{
	int i=0;
	misc_deregister(&gec6818_key_miscdev);
	for(i=0;i<4;++i)
		gpio_free(key_info_tab[i].num);
	printk("removed key_drv\n");
}

module_init(gec6818_key_init);
module_exit(gec6818_key_exit);

MODULE_AUTHOR("yonghui.lee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 KEY Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v2.0");

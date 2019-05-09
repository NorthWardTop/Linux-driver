/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 本文件为LED驱动程序
 * 使用ioctl实现应用层指令控制驱动程序
 * 实现: 按下K点亮LED
 * 驱动层:
 * 		注册一个杂项设备, 解析ioctl的指令, 
 * 		并读K值(copy_to_user)或写LED值(copy_from_user)
 * 应用层:
 * 		打开设备LED和K, 使用ioctl执行指令去读K设备值或写LED设备值
 *		
 * @Date: 2019-05-01 15:47:55
 * @LastEditTime: 2019-05-10 00:18:59
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
#include <linux/ioport.h>


#define GEC6818_ADC_IN0			_IOR('A', 1, unsigned long)
#define GEC6818_ADC_IN1			_IOR('A', 2, unsigned long)

#define DEV_NAME				"ADC"



// static struct cdev gec6818_adc_cdev;
// static dev_t dev_num;
// static struct class *adc_class;
// static struct device *adc_device;



static void __iomem *adc_base_va; //基地址
static void __iomem *adc_con_va; //控制寄存器
static void __iomem *adc_dat_va;
static void __iomem *adc_pre_va; //预分频寄存器


// struct led_gpio_info{
// 	unsigned int num;
// 	char name[16];
// };

// static const struct led_gpio_info led_info_tab[4]={
// 	{.num=PAD_GPIO_E+13, .name="gpioe13"},
// 	{.num=PAD_GPIO_C+7, .name="gpioc7"},
// 	{.num=PAD_GPIO_C+8, .name="gpioc8"},
// 	{.num=PAD_GPIO_C+17, .name="gpioc17"},
// };


long gec6818_adc_ioctl(struct file *fl, unsigned int cmd, unsigned long arg)
{
	//获取当前LED灯下标
	unsigned long index=arg-7;
	switch(cmd)
	{
		case GEC6818_LED_ON:
			gpio_set_value(led_info_tab[index].num, 0);
			break;
		case GEC6818_LED_OFF:
			gpio_set_value(led_info_tab[index].num, 1);
			break;
		default:
			return -ENOIOCTLCMD;
	}
	return 0;
}




//打开
static int gec6818_adc_open(struct inode *ino, struct file *f)
{
	int i=0;
	//配置对应gpio为输出模式
	for(;i<4;++i)
		gpio_direction_output(led_info_tab[i].num, 1);
	printk("LED device opened!\n");
	return 0;
}

//关闭
static int gec6818_adc_release(struct inode *i, struct file *f)
{
	printk("LED closed\n");
	return 0;
}

static const struct file_operations gec6818_adc_fops={
	.owner		= THIS_MODULE,
	.write		= gec6818_adc_write,
	.read 		= gec6818_adc_read,
	.open		= gec6818_adc_open,
	.release 	= gec6818_adc_release,
	.unlocked_ioctl = gec6818_adc_ioctl
};

static struct miscdevice gec6818_adc_miscdev={
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVICE_NAME,
	.fops		= &gec6818_adc_fops
};

static int __init gec6818_adc_init(void)
{
	int ret = 0;
	struct resource *adc_res;
	//动态申请设备号
	ret = alloc_chrdev_region(&adc_num, 0, 1, DEV_NAME);
	if(ret<0) {
		printk("alloc_chrdev_region failed\n");
		return ret;
	}
	//字符设备初始化
	cdev_init(&gec6818_adc_cdev, &gec6818_adc_fops);
	//字符设备添加到内核
	ret = cdev_add(&gec6818_adc_cdev, adc_num, 1);
	if (ret < 0) {
		printk("cdev_add failed\n");
		goto cdev_add_fail;
	} 
	//创建类
	adc_class = class_create(THIS_MODULE, DEV_NAME);
	if(IS_ERR(adc_class)) {
		ret = PTR_ERR(adc_class);
		printk("class_create failed\n");
		goto class_create_fail;
	}
	//创建设备
	adc_device = device_create(adc_class, NULL, adc_num, NULL, DEV_NAME);
	if(ISERR(adc_device)) {
		ret = PTR_ERR(adc_device);
		printk("device_create fail\n");
		goto device_create_fail;
	}
	//申请物理地址0-10+10-13 = 0x14
	adc_res = request_mem_region(0xC0053000, 0x14, "ADC_MEM");
	if(!adc_res) {
		ret = -ENOMEM;
		printk("request_mem_region fail\n");
		goto request_mem_region_fail;
	}
	//物理地址映射到虚拟地址
	adc_base_va = ioremap(0xC0053000, 0x14);
	if (!adc_base_va) {
		ret = -ENOMEM;
		printk("ioremap failed\n");
		goto ioremap_fail;
	}

	adc_con_va = adc_base_va+0x00;
	adc_dat_va = adc_base_va+0x04;
	adc_pre_va = adc_base_va+0x10;

	printk("ADC device init\n");
	return 0;
ioremap_fail:
	//取消物理地址请求
	release_mem_region(0xC0053000, 0x14);
request_mem_region_fail:
	//取消设备注册
	device_destroy(adc_class, adc_num);
device_create_fail:
	//取消类注册
	class_destroy(adc_class, adc_num);
class_create_fail:
	//删除内核设备
	cdev_del(&gec6818_adc_cdev);
cdev_add_fail:
	//取消申请的设备号,设备号, 释放数量
	unregister_chrdev_region(adc_num, 1);
	return ret;
}

static void __exit gec6818_adc_exit(void)
{
	iounmap(adc_base_va);
	misc_deregister(&gec6818_adc_miscdev);
	printk("gec6818 adc exit\n");
}

module_init(gec6818_adc_init);
module_exit(gec6818_adc_exit);

MODULE_AUTHOR("yonghui.lee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 ADC Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v3.0");

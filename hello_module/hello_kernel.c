/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 
 * printk文件内容:各种消息的优先级
 * 1.控制台打印级别:当默认消息级别高于该值时,消息就能打印
 * 2.默认消息日志级别:基准
 * 
 * 
 * @Date: 2019-04-27 19:59:54
 * @LastEditTime: 2019-04-28 15:14:00
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME		"led"





// static struct miscdev led_dev=
// {
// 	.minor=MISC_DYNAMIC_MINOR,
// 	.name=DEVICE_NAME,
// 	.fops=&led_dev_fops,
// };

static int __init led_init(void)
{
	//while(1);
	printk("fffffffGEC6818 LED installed, (c) 2019 SLsiAP\n");
	return 0;
}

static void __exit led_exit(void)
{
	printk("GEC6818 LED removed, (c) 2019 SLsiAP\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("yonghuilee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");
//MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
//MODULE_ALIAS("platform:nxp-wdt");


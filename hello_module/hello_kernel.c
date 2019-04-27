/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: Description
 * @Date: 2019-04-27 19:59:54
 * @LastEditTime: 2019-04-27 22:16:08
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>








static int __init led_init(void)
{
	while(1);
	printk("GEC6818 LED installed, (c) 2019 SLsiAP\n");
	return 0;
}

static void __exit led_exit(void)
{
	printk(KERN_WARNING"GEC6818 LED removed, (c) 2019 SLsiAP\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("yonghuilee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");
//MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
//MODULE_ALIAS("platform:nxp-wdt");

